#include "../generated.h"
#include "../lcd2004/lcd2004.h"
#include "../serial/serial.h"
#include "math.h"
#include <stdint.h>

// Reserve one bit for command/data mode
#define COLORS_PER_BYTE ((8 - 1) / BITS_PER_COLOR)

// Colors store example:
//   x0  x1  x2  x3  x4  x5  x6  x7
//
//   1   2   3   4   1   2   3   4   y0
//   5   6   7   8   5   6   7   8   y1
//   9  10  11  12   9  10  11  12   y2
//
// 1.2.3.4     1.2.3.4
// 5.6.7.8     5.6.7.8
// 9.10.11.12  9.10.11.12

// Float/int relationship in the canvas:
//
//                                   SCREENX
//      |       |       |       |       |
//      0       1       2       3       4
//      0.0     1.0        2.5          4.0
//
// C cast in positive integers does a floor operation; 2.5 becomes 2 and paints the correct pixel


typedef uint8_t canvas_t[SCREENY][SCREENX / COLORS_PER_BYTE];
// Never used; useful to check the size
const uint16_t ARRAY_SIZE = sizeof(canvas_t);

// Create a struct to send the command together with the actual data
typedef struct {
    BACKEND_TO_FRONTEND start_command;
    volatile canvas_t   canvas;
    BACKEND_TO_FRONTEND end_command;
} frame_message_t;

volatile frame_message_t frame_message_1;


// Values are the colors
typedef enum __attribute((__packed__)) {
    PROJ           = 1,
    PARACHUTE      = 2,
    CANNON_POINTER = 3,
} entity_variant_t;

typedef struct {
    entity_variant_t variant;
    float            pos_x;
    float            pos_y;
    float            speed_x;
    float            speed_y;
} entity_t;

// Speeds are declared in display%/sec, converted to px/sec
#define SPEED_UNIT         (SCREENX / 100.f)
#define RECHARGE_TIME_MS   100
#define INITIAL_PROJ_SPEED (60.0f * SPEED_UNIT)
#define PARACHUTE_SPEED    (-10.0f * SPEED_UNIT)
#define G                  9.81f
#define PARACHUTE_SPAWN_MS 1000

#define MAX_ENTITIES_LEN 50
entity_t entities[MAX_ENTITIES_LEN];
uint8_t  entities_len       = 0;
uint32_t last_tick          = 0;
uint32_t last_shot_ms       = 0;
uint32_t last_chute_spawned = 0;

#define RANDOM_LEN 20
uint8_t randoms[RANDOM_LEN] = {12, 85, 3,  67, 91, 28, 54, 76, 19, 43,
                               8,  62, 37, 89, 15, 71, 46, 23, 58, 94};

void init_game() {
    entities[0].variant = CANNON_POINTER;

    entities_len = 1;
}

void delete_entity(uint8_t index) {
    entities[index] = entities[entities_len - 1];
    entities_len--;
}

uint8_t spawn_entity_non_init() {
    if (entities_len >= MAX_ENTITIES_LEN) {
        throw_error(GAME_MAX_ENTITIES_REACHED);
    }
    return entities_len++;
}

void process_tick(uint32_t current_ms, float angle_rad, boolean shoot_pressed) {
    lcd_set_cursor(3, 0);
    lcd_write_uint16(angle_rad);
    if (last_tick == 0) {
        last_tick = current_ms;
        return;
    }

    float delta_seconds = (current_ms - last_tick) / 1000.0;
    last_tick           = current_ms;

    // angle_rad             = fmaxf(0, fminf(angle_rad, M_PI));
    float aim_component_x = cosf((float) angle_rad);
    float aim_component_y = sinf((float) angle_rad);

    float len         = ((float) SCREENX) / 5;
    entities[0].pos_x = SCREENX / 2.0 + aim_component_x * len;
    entities[0].pos_y = SCREENY * 1.0 - fmaxf(aim_component_y, 0) * len;


    // Process physics, skip cannon
    for (uint8_t i = 1; i < entities_len; i++) {
        // Apply gravity only to projectiles
        if (entities[i].variant == PROJ) {
            entities[i].speed_y -= G * delta_seconds;
        }

        // Update position based on velocity
        entities[i].pos_x += entities[i].speed_x * delta_seconds;
        entities[i].pos_y -= entities[i].speed_y * delta_seconds;

        // Check bounds and hide entity if out of screen
        if (entities[i].pos_x < 0 || entities[i].pos_x >= SCREENX || entities[i].pos_y < 0 ||
            entities[i].pos_y >= SCREENY) {
            delete_entity(i);
            i--;
        }
    }

    if (last_chute_spawned + PARACHUTE_SPAWN_MS < current_ms) {
        uint8_t index = spawn_entity_non_init();

        last_chute_spawned = current_ms;

        entities[index] =
            (entity_t) {.variant = PARACHUTE,
                        // Cannon as initial pos
                        .pos_x   = randoms[current_ms % RANDOM_LEN] * (uint8_t) SCREENX / 100.,
                        .pos_y   = 1,
                        .speed_x = 0,
                        .speed_y = PARACHUTE_SPEED};
    }

    // Shoot?
    if (shoot_pressed && last_shot_ms + RECHARGE_TIME_MS < current_ms) {
        uint8_t index = spawn_entity_non_init();

        // Shoot!
        last_shot_ms = current_ms;

        entities[index] = (entity_t) {.variant = PROJ,
                                      // Cannon as initial pos
                                      .pos_x   = entities[0].pos_x,
                                      .pos_y   = entities[0].pos_y,
                                      .speed_x = INITIAL_PROJ_SPEED * aim_component_x,
                                      .speed_y = INITIAL_PROJ_SPEED * aim_component_y};
    }
}

// Choose
void render() {
    volatile canvas_t *canvas = &frame_message_1.canvas;

    // Clear canvas
    for (uint8_t i = 0; i < sizeof(canvas_t); i++) {
        ((volatile uint8_t *) (*canvas))[i] = 0;
    }

    for (uint8_t i = 0; i < entities_len; i++) {
        uint8_t x     = (uint8_t) entities[i].pos_x;
        uint8_t y     = (uint8_t) entities[i].pos_y;
        uint8_t color = entities[i].variant; // By definition variant value is the color

        uint8_t shifted_color = color << (BITS_PER_COLOR * (x % COLORS_PER_BYTE));

        (*canvas)[y][x / COLORS_PER_BYTE] |= shifted_color;
    }
}


void start_sending_frame() {
    frame_message_1.start_command = SET_COMMAND(FRAME_START);
    frame_message_1.end_command   = SET_COMMAND(FRAME_END);


    send_data((uint8_t *) &frame_message_1, sizeof(frame_message_t));
}

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

typedef struct {
    uint8_t color;
    uint8_t x_pos;
    uint8_t y_pos;
} colored_pixels_t;

colored_pixels_t colored_pixels[MAX_ENTITIES_LEN];
uint8_t          num_drawable_pixels;

// Tracks the sent pixel
uint8_t current_pixel_idx = 0;
// Tracks the state of sending a frame
uint8_t frame_send_status;
// Used as the current row index (Y-coordinate on screen) during frame sending
uint8_t x_send_status;
// Used as the current byte column index in the current row during frame sending
uint8_t y_send_status;

volatile boolean generator_f(uint8_t *data) {
    switch (frame_send_status) {
        case 0:
            *data = SET_COMMAND(FRAME_START);
            frame_send_status++;
            x_send_status = 0; // current_row
            y_send_status = 0; // current_byte_col
            return true;

        case 1: { // Sending frame data
            // Check if all rows have been rendered and sent
            if (x_send_status == SCREENY) {
                *data = SET_COMMAND(FRAME_END);
                frame_send_status++; // Move to next state (typically done/default)
                return true;
            }

            uint8_t current_row      = x_send_status;
            uint8_t current_byte_col = y_send_status;
            uint8_t byte_to_send     = 0; // Initialize to 0 (all data bits 0, MSB command bit 0)

            // Iterate through the sorted colored_pixels to find pixels for the current byte
            // current_pixel_idx points to the next pixel in colored_pixels to consider
            while (current_pixel_idx < num_drawable_pixels) {
                colored_pixels_t *pixel = &colored_pixels[current_pixel_idx];

                if (pixel->y_pos < current_row) {
                    // This pixel is for a previous row that we've already passed. Skip it.
                    current_pixel_idx++;
                    continue;
                }

                if (pixel->y_pos > current_row) {
                    // This pixel (and all subsequent ones due to sorting) are for future rows.
                    // So, no more contributions from colored_pixels for the current_row.
                    // The current byte_to_send (which is likely 0) is complete for this (row,
                    // byte_col).
                    break;
                }

                // At this point, pixel->y_pos == current_row. Check its byte column.
                // Calculate which byte column this pixel falls into.
                uint8_t pixel_byte_col_for_pixel = pixel->x_pos / COLORS_PER_BYTE;

                if (pixel_byte_col_for_pixel < current_byte_col) {
                    // This pixel is for a previous byte column in the current_row. Skip it.
                    current_pixel_idx++;
                    continue;
                }

                if (pixel_byte_col_for_pixel > current_byte_col) {
                    // This pixel (and others after it on this row, if any) are for future byte
                    // columns. So, no more contributions from colored_pixels for the
                    // current_byte_col. The current byte_to_send is complete.
                    break;
                }

                // At this point, pixel->y_pos == current_row AND
                // pixel_byte_col_for_pixel == current_byte_col.
                // This means the pixel at colored_pixels[current_pixel_idx]
                // belongs in the current byte_to_send!

                uint8_t color_value = pixel->color;
                // Determine the pixel's position *within* the current byte
                // e.g., is it the 0th, 1st, or Nth color slot in this byte
                uint8_t pixel_pos_in_byte = pixel->x_pos % COLORS_PER_BYTE;

                // Shift the color into the correct bit position(s) within the byte.
                // This assumes BITS_PER_COLOR and COLORS_PER_BYTE are set up so that
                // (color_value << (BITS_PER_COLOR * pixel_pos_in_byte)) fits within
                // the bits allocated for color data (e.g., lower 7 bits if MSB is command).
                uint8_t shifted_color = color_value << (BITS_PER_COLOR * pixel_pos_in_byte);

                byte_to_send |= shifted_color; // Combine with any other pixels in this byte

                current_pixel_idx++; // Consume this pixel, move to the next in colored_pixels
                                     // to check if it also fits in the current byte_to_send.
            }

            *data = byte_to_send; // Assign the fully composed byte (MSB should be 0 for data)

            // Advance iterators to the next byte position in the frame
            y_send_status++; // Move to the next byte column in the current row
            if (y_send_status ==
                (SCREENX / COLORS_PER_BYTE)) { // Reached the end of byte columns for this row
                y_send_status = 0; // Reset byte column index to the beginning of a new row
                x_send_status++;   // Move to the next row
            }
            return true; // Indicate that this data byte is valid
        }

        default:
            return false;
    }
}

void start_sending_frame() {
    num_drawable_pixels = 0; // Reset count for the current frame

    // 1. Initialize colored_pixels from entities
    // Iterate through all active entities
    for (uint8_t i = 0; i < entities_len; i++) {
        // Safety break: ensure we don't write out of bounds for colored_pixels.
        // This should not be hit if MAX_ENTITIES_LEN is consistent.
        if (num_drawable_pixels >= MAX_ENTITIES_LEN) {
            break;
        }

        uint8_t pixel_x = (uint8_t) entities[i].pos_x;
        uint8_t pixel_y = (uint8_t) entities[i].pos_y;

        // Ensure pixels are within screen boundaries after casting
        if (pixel_x < SCREENX && pixel_y < SCREENY) {
            colored_pixels[num_drawable_pixels].x_pos = pixel_x;
            colored_pixels[num_drawable_pixels].y_pos = pixel_y;
            colored_pixels[num_drawable_pixels].color = entities[i].variant;
            num_drawable_pixels++;
        }
    }

    // 2. Sort colored_pixels by y_pos, then by x_pos (Bubble Sort)
    //    Only sort the valid part of the array, i.e., up to num_drawable_pixels.
    if (num_drawable_pixels > 1) { // Only sort if there's more than one element
        for (uint8_t i = 0; i < num_drawable_pixels - 1; i++) {
            for (uint8_t j = 0; j < num_drawable_pixels - 1 - i; j++) {
                // Compare y_pos first
                if (colored_pixels[j].y_pos > colored_pixels[j + 1].y_pos ||
                    // If y_pos is the same, compare x_pos
                    (colored_pixels[j].y_pos == colored_pixels[j + 1].y_pos &&
                     colored_pixels[j].x_pos > colored_pixels[j + 1].x_pos)) {
                    // Swap elements
                    colored_pixels_t temp = colored_pixels[j];
                    colored_pixels[j]     = colored_pixels[j + 1];
                    colored_pixels[j + 1] = temp;
                }
            }
        }
    }

    // Reset frame send status and the current_pixel_idx for generator_f
    frame_send_status = 0;
    current_pixel_idx = 0; // This is the 'global pointer' for generator_f to iterate colored_pixels
    send_data_generator_f(generator_f);
}

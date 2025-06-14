#include "analog/analog.h"
#include "game/game.h"
#include "generated.h"
#include "lcd2004/lcd2004.h" // For the character LCD
#include "ports.h"
#include "serial/serial.h"
#include "timers/timer.h"
#include "two_wires/tw.h"
#include "utils/utils.h"
#include <math.h>
#include <stdint.h>

#define PCICR EXPAND_ADDRESS(0x68)
// uint8_t *const EICRA = (uint8_t *)0x69;
#define PCMSK0 EXPAND_ADDRESS(0x6B)
#define PCMSK1 EXPAND_ADDRESS(0x6C)

INTERRUPT(default) {
    throw_error(BAD_INTERRUPT);
}


int main(void) {
    init_blinks();

    // To see bootloop
    on_blue();
    wait();
    off_blue();

    init_errors();
    init_timer0();
    init_sleep();
    init_ADC();
    init_USART();
    init_two_wires();
    init_lcd_2004(); // Requires 2 wires

    init_game();

    // Useful to find the display
    // scan_i2c_addresses();


    // Enable global interrupts
    manage_global_interrupts(true);


    uint8_t status = SET_COMMAND(BOOTED);
    send_data(&status, 1);
    serial_out_join();

    BIT_NO(GAME_SHOOT_PIN, 4);

    CLEAR_BIT(DDRD, GAME_SHOOT_PIN);
    // Enable pullup
    SET_BIT(PORTD, GAME_SHOOT_PIN);

    uint32_t last_frame_plus_render_time = 0;
    uint32_t last_logic_time             = 0;
    uint32_t last_total_time             = 0;

    while (1) {
        uint16_t angle = analog_read_pin_sync(1);


        uint16_t max_angle = (1 << 10) - 1;
        float    angle_rad = ((float) (angle)) / (max_angle) *M_PI;

        boolean pressed = !(PIND & (1 << GAME_SHOOT_PIN));


        process_tick(get_current_time(), angle_rad, pressed);

        start_sending_frame();
        serial_out_join();

        uint8_t values[4] = {
            SET_COMMAND(SCORE),
            SET_DATA(score),
            SET_COMMAND(BULLETS),
            SET_DATA(bullets),
        };

        send_data(values, 4);
        serial_out_join();
        // sleep_ms(1000);

        return 0;
    }
}

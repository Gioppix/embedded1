#include "analog/analog.h"
#include "lcd2004/lcd2004.h"
#include "ports.h"
#include "serial/serial.h"
#include "software-serial/sw-serial.h"
#include "timers/timer.h"
#include "two_wires/tw.h"
#include "utils/utils.h"
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
    init_sw_serial();

    // Userful to find the display
    // scan_i2c_addresses();

    init_lcd_2004(); // Requires 2 wires


    // Enable global interrupts
    manage_global_interrupts(true);

    uint8_t col     = 0;
    uint8_t max_col = 19;
    uint8_t row     = 0;
    uint8_t max_row = 3;

    // send_byte('A');
    // send_byte('T');
    // send_byte('+');
    // send_byte('N');
    // send_byte('A');
    // send_byte('M');
    // send_byte('E');
    // send_byte('D');
    // send_byte('E');
    // send_byte('S');
    // send_byte('I');
    // send_byte('R');
    // send_byte('E');
    // send_byte('D');
    // send_byte(' ');
    // send_byte('N');
    // send_byte('A');
    // send_byte('M');
    // send_byte('E');

    // send_byte('\r');
    // send_byte('\n');

    // send_byte('A');
    // send_byte('T');
    // send_byte('+');
    // send_byte('R');
    // send_byte('E');
    // send_byte('S');
    // send_byte('E');
    // send_byte('T');
    // send_byte('\r');
    // send_byte('\n');

    while (1) {
        throw_error_if_present(lcd_clean());
        throw_error_if_present(lcd_set_cursor(row, col));
        throw_error_if_present(lcd_write_string("a"));

        row++;
        if (row > max_row) {
            row = 0;
            col++;
            col %= (max_col + 1);
        }

        boolean s = send_byte('a');
        println_num(deb);

        sleep_ms(250);
    }

    return 0;
}

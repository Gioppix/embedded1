#include "lcd2004.h"
#include "../timers/timer.h"
#include <stdint.h>

// https://www.sainsmart.com/products/sainsmart-sensor-shield-v4-module-lcd2004-for-arduino-uno-mega-r3-atmel-avr?srsltid=AfmBOopzivSkD3-V1lSFcbC2ND8my07axUMZqqNYRO8KlpP6IX-nvSFE
#define DISPLAY_I2C_ADDRESS 0x27

#define COLS 20
#define ROWS 4


// PCF8574 Pin | LCD HD44780 Pin | Function
// P0          | RS              | Register Select          (0:instruction, 1:data)
// P1          | Enable          | Enable Signal            (Starts data read/write)
// P2          | D4              | Data Bit 4                -
// P3          | D5              | Data Bit 5               | Used in 4 bit operation
// P4          | D6              | Data Bit 6               |
// P5          | D7              | Data Bit 7                -
// P6          | -               | Not used (or backlight)
// P7          | Backlight       | LCD Backlight Control

// I/D = 1: Increment
// I/D = 0: Decrement
// S = 1: Accompanies display shift
// S/C = 1: Display shift
// S/C = 0: Cursor move
// R/L = 1: Shift to the right
// R/L = 0: Shift to the left
// DL = 1: 8 bits, DL = 0: 4 bits
// N = 1: 2 lines, N = 0: 1 line
// F = 1: 5 × 10 dots, F = 0: 5 × 8 dots
// BF = 1: Internally operating
// BF = 0: Instructions acceptable

// https://cdn.sparkfun.com/assets/9/5/f/7/b/HD44780.pdf#page=24

void throw_error_if_present(ERROR err) {
    if (err) {
        throw_error(err);
    }
}

// rs 0:instruction, 1:data
void lcd_send_2_nibbles(uint8_t cmd, boolean rs) {
    uint8_t high_nibble = (cmd & 0xF0) | 0x08 | rs;        // High nibble + backlight + maybe rs
    uint8_t low_nibble  = ((cmd << 4) & 0xF0) | 0x08 | rs; // Low nibble + backlight + maybe rs

    // Send both nibbles with enable pulses.
    // The enable bit latches the signal (or something)
    uint8_t sequence[6];
    sequence[0] = high_nibble;
    sequence[1] = high_nibble | 0x04;  // Set enable bit
    sequence[2] = high_nibble & ~0x04; // Clear enable bit
    sequence[3] = low_nibble;
    sequence[4] = low_nibble | 0x04;  // Set enable bit
    sequence[5] = low_nibble & ~0x04; // Clear enable bit

    write_two_wires_start(DISPLAY_I2C_ADDRESS, sequence, 6);
}


void write_char_lcd_2004(uint8_t charr) {
    lcd_send_2_nibbles(charr, true);
}

void lcd_clean() {
    lcd_send_2_nibbles(0b1, false);
    sleep_ms(5);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    // Validate bounds
    if (row >= ROWS || col >= COLS) {
        throw_error(LCD_INVALID_ROW_OR_COL);
    }


    uint8_t address;

    // Calculate DDRAM address based on row
    switch (row) {
        case 0:
            address = 0x00 + col;
            break; // Line 0
        case 1:
            address = 0x40 + col;
            break; // Line 1
        case 2:
            address = 0x14 + col;
            break; // Line 2
        case 3:
            address = 0x54 + col;
            break; // Line 3
    }

    // Set DDRAM Address command: 1AAAAAAA (bit 7 = 1, bits 6-0 = address)
    lcd_send_2_nibbles(0x80 | address, false);

    sleep_ms(5);
}


void lcd_write_string(const char *text) {
    while (*text) {
        write_char_lcd_2004(*text);
        sleep_ms(1);
        text++;
    }
}

void lcd_write_uint16(uint16_t value) {
    char buffer[6]; // Max 5 digits + null terminator for uint16_t (0-65535)
    int  i = 0;

    // Handle zero case
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
    } else {
        // Convert to string (reverse order)
        uint16_t temp = value;
        while (temp > 0) {
            buffer[i++] = '0' + (temp % 10);
            temp /= 10;
        }
        buffer[i] = '\0';

        // Reverse the string
        for (int j = 0; j < i / 2; j++) {
            char temp_char    = buffer[j];
            buffer[j]         = buffer[i - 1 - j];
            buffer[i - 1 - j] = temp_char;
        }
    }

    lcd_write_string(buffer);
}

void init_lcd_2004() {
    // Power stabilization delay, apparently important
    sleep_ms(50);

    uint8_t current        = 0;
    uint8_t lcd_commands[] = {
        // 3-command reset sequence: https://cdn.sparkfun.com/assets/9/5/f/7/b/HD44780.pdf#page=45
        // This will always work, both 4 and 8 bit modes
        0x33,
        0x33,
        0x33,
        0x33,
        0x33,
        0x33,
        // Switch to 4-bit mode
        0x20,
        // Function set
        // 1 DL N F - -
        0b00100000,
        // Display off
        0b1000,
        // Display clean
        0b1,
        // Entry mode set (increment, no shift)
        0b110,
        // Display on, cursor, blink
        // 1 D C B
        0b1100,
        // Detect end, not a valid command
        0};


    while (lcd_commands[current]) {
        lcd_send_2_nibbles(lcd_commands[current], false);
        sleep_ms(10);
        current++;
    }

    throw_error_if_present(write_two_wires_join());
}

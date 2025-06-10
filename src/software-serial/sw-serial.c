#include "sw-serial.h"
#include "../gen_queue.h"
#include <stdint.h>

volatile boolean sending = false;
volatile uint8_t sending_bit;
volatile uint8_t bit_to_send = 0;

volatile uint8_t skip_tick = true;

volatile uint8_t deb;

DECLARE_QUEUE(sw_serial, volatile uint8_t, volatile uint8_t, 250)

// Pin state; 0:low 1:high
#define PIND EXPAND_ADDRESS(0x29)
// Data dir; 0:in 1:out
#define DDRD EXPAND_ADDRESS(0x2A)
// If output: 0:low 1:high
#define PORTD EXPAND_ADDRESS(0x2B)

#define RX 2
#define TX 3


void init_sw_serial() {
    // Set output
    SET_BIT(DDRD, TX);

    // Pull high TX
    SET_BIT(PORTD, TX);
}

boolean send_byte(uint8_t byte) {
    return sw_serial_enqueue(byte);
}

inline void process_sw_serial_tick() {
    skip_tick = !skip_tick;
    if (skip_tick) {
        return;
    }

    if (sending) {
        deb++;
        if (bit_to_send < 8) {
            MANAGE_BIT(PORTD, TX, (sending_bit >> bit_to_send) & 0b1);
            bit_to_send++;
        } else {
            // Stop bit
            SET_BIT(PORTD, TX);
            sending = false;
        }
    } else {
        boolean success = sw_serial_dequeue(&sending_bit);

        if (success) {
            sending     = true;
            bit_to_send = 0;

            // Send start bit
            CLEAR_BIT(PORTD, TX);
        }
    }
};

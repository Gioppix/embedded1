#include "utils.h"
#include "../ports.h"

#define SMCR EXPAND_ADDRESS(0x53)
BIT_NO(SE, 0);
BIT_NO(SM0, 1);

// 3 bits, SM0..2
// This is idle state, so that every interrupt wakes up the CPU
#define SLEEP_MODE 0b000


void init_errors() {
    init_blinks();
}

// Busy wait for a while, no need for interrupts
void wait() {
    for (unsigned int i = 0; i < 10000; i++)
        for (int j = 0; j < 10; j++)
            ;
}

void throw_error(ERROR error_kind) {
    manage_global_interrupts(false);
    while (1) {
        for (uint8_t i = 0; i < error_kind; i++) {
            SET_BIT(PORTB, 1);
            on_red();
            wait();
            off_red();
            wait();
        }
        for (uint8_t i = 0; i < 5; i++) {
            wait();
        }
    }
}

void init_sleep() {
    // Enables sleep mode.
    // No need to disable it afterwards for safety
    SET_BIT(SMCR, SE);

    SMCR |= SLEEP_MODE << SM0;
}

void sleep() {
    asm("sleep");
}


void init_blinks() {
    SET_BIT(DDRB, 0);
}

void short_blink() {
    on_green();
    wait();
    off_green();
    wait();
}

#include "utils.h"
#include "../ports.h"

#define SMCR EXPAND_ADDRESS(0x53)
BIT_NO(SE, 0);
BIT_NO(SM0, 1);

// 3 bits, SM0..2
// This is idle state, so that every interrupt wakes up the CPU
#define SLEEP_MODE 0b000

void init_errors() {
    // Show bootloop
    SET_BIT(DDRB, 1);
}

void wait() {
    for (unsigned int i = 0; i < 30000; i++)
        for (int j = 0; j < 10; j++)
            ;
}

void throw_error(ERROR error_kind) {
    manage_global_interrupts(false);
    while (1) {
        SET_BIT(PORTB, 1);
        wait();
        CLEAR_BIT(PORTB, 1);
        wait();
    }
}

void init_sleep() {
    // Enables sleep mode.
    // No need to disable it for safety
    SET_BIT(SMCR, SE);

    SMCR |= SLEEP_MODE << SM0;
}

void sleep() { asm("sleep"); }

void init_short_blink() { SET_BIT(DDRB, 0); }

void short_blink() {
    SET_BIT(PORTB, 0);
    wait();
    CLEAR_BIT(PORTB, 0);
    wait();
}

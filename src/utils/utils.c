#include "utils.h"
#include "../ports.h"

void init_errors() {
    // Show bootloop
    SET_BIT(DDRB, 1);
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

void wait() {
    for (unsigned int i = 0; i < 30000; i++)
        for (int j = 0; j < 10; j++)
            ;
}

void init_short_blink() { SET_BIT(DDRB, 0); }

void short_blink() {
    SET_BIT(PORTB, 0);
    wait();
    CLEAR_BIT(PORTB, 0);
    wait();
}

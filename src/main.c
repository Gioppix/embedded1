#include "ports.h"
#include "serial/serial.h"
#include "timers/timer.h"
#include "utils/utils.h"
#include <stdint.h>

#define PCICR EXPAND_ADDRESS(0x68)
// uint8_t *const EICRA = (uint8_t *)0x69;
#define PCMSK0 EXPAND_ADDRESS(0x6B)
#define PCMSK1 EXPAND_ADDRESS(0x6C)

INTERRUPT(default) { throw_error(BAD_INTERRUPT); }

int main(void) {
    init_errors();

    // Show bootloop
    SET_BIT(DDRB, 1);
    SET_BIT(PORTB, 1);
    wait();
    CLEAR_BIT(PORTB, 1);

    init_USART(MYUBRR);

    // Enable global interrupts
    manage_global_interrupts(true);

    init_short_blink();
    init_timer0();

    uint32_t i = 0;
    while (1) {
        if (get_current_time() > i * 1000) {
            boolean res = print_num(i);
            i++;
        }
    }

    return 0;
}

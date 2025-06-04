#include "analog/analog.h"
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
    init_timer0();
    init_short_blink();
    init_sleep();
    init_ADC();
    init_USART();

    // To see bootloop
    SET_BIT(DDRB, 1);
    SET_BIT(PORTB, 1);
    sleep_ms(100);
    CLEAR_BIT(PORTB, 1);

    // Enable global interrupts
    manage_global_interrupts(true);

    while (1) {
        uint16_t res = analog_read_pin_sync(1);
        println_num(res);
        sleep_ms(100);
    }

    return 0;
}

#include "timer.h"

// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=87
#define TCCR0A EXPAND_ADDRESS(0x44)
#define TCCR0B EXPAND_ADDRESS(0x45)
#define TCNT0  EXPAND_ADDRESS(0x46)
#define OCR0A  EXPAND_ADDRESS(0x47)
#define TIMSK0 EXPAND_ADDRESS(0x6E)

// (prescaler * number_to_reach * number_of_reaches) / freq = time_elapsed
// (8 * x * 10) / 16000000 = 1/1000
// x = 200
// x is the compare register value: If I gen an interrupt `number_of_reaches`
// times that means a ms has elapsed.
//
// We need to tweak the values so that interrupts are as sparse as possible;
// losing one means losing a tick. Other example values: (1 * 64 * 250) /
// 16000000 = 1/1000, 1 interrupt every 0.004 secs, too fast.
//
// Another option:
// (64 * x * 1) / 16000000 = 1/1000 (one interrupt per ms)
//
// Current numbers give us an interrupt every 0.1 ms, very convenient
//
// Numbers must be integers (duh)
#define MATCH_A        200
#define TIMES_TO_MATCH 10
// For register TCCR0B
#define PRESCALER_BITS 0b010

// ~4 million msecs
volatile uint32_t current_time = 0;

volatile uint8_t times_match_reached = 0;

// Interrupts (MUST DO N-1!!! THEY ARE 0-BASED):
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=49

// Timer/Counter0 match A
INTERRUPT(14) {
    times_match_reached++;
    if (times_match_reached == TIMES_TO_MATCH) {
        current_time++;
        times_match_reached = 0;
    }
}

void init_timer0() {
    OCR0A = MATCH_A;

    // https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=86
    // Enable clear on compare match - manual clear is too slow for no prescaler
    SET_BIT(TCCR0A, 1);

    // Enable overflow interrupt (should not happen, will call bad_interrupt so
    // that we know something bad happened)
    SET_BIT(TIMSK0, 0);
    // Enable Compare A interrupt
    SET_BIT(TIMSK0, 1);

    // Set prescaler. Enables timer
    TCCR0B |= PRESCALER_BITS;
}

void sleep_ms(uint32_t ms) {
    uint32_t start_time = get_current_time();
    while (get_current_time() - start_time < ms) {
        sleep();
    }
}

// This is needed as 32bits operations are not atomic
inline uint32_t get_current_time() {
    uint32_t local_current;
    CRITICAL {
        local_current = current_time;
    }
    return local_current;
}

#include "timer.h"
#include "../software-serial/sw-serial.h"

// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=87
#define TCCR0A EXPAND_ADDRESS(0x44)
#define TCCR0B EXPAND_ADDRESS(0x45)
#define TCNT0  EXPAND_ADDRESS(0x46)
#define OCR0A  EXPAND_ADDRESS(0x47)
#define TIMSK0 EXPAND_ADDRESS(0x6E)

// (prescaler * number_to_reach * number_of_reaches) / freq = time_elapsed
//
// Example:
// (8 * x * 10) / 16000000 = 1/1000
// x = 200
// x is the compare register value: If I gen an interrupt `number_of_reaches`
// times that means a ms has elapsed.
//
//
// Another option:
// (64 * x * 1) / 16000000 = 1/1000 (one interrupt per ms)
//
// We need to tweak the values so that interrupts are as sparse as possible;
// losing one means losing a tick. But there's also another consideration: Software serial needs
// microsecond precision.
// Example for 9600 baud: 1/9600*1000*1000 = ~104 micros for one bit, but since we also need to
// wait 1.5t on rx we need ~52 micros accuracy.
//
// Let's make it so we get an interrupt every 52 micros:
// (64 * x * 1) / 16000000 = 1/1000/1000*52
// x = 13
//
// Numbers must be integers (duh)

// Minus one since it starts from 0 and it compares for equality
#define MATCH_A (13 - 1)
// For register TCCR0B
#define PRESCALER_BITS 0b011

// ~4 million msecs range, uint16_t too small
volatile uint32_t current_ms = 0;

volatile uint8_t current_ticks = 0;
// Count number of times we approximate to adjust later
volatile uint16_t count_40_micros_lost = 0;


// Interrupts (MUST DO N-1!!! THEY ARE 0-BASED in avr-gcc, 1-based in docs):
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=49

// Timer/Counter0 match A
INTERRUPT(14) {
    current_ticks++;
    if (current_ticks == 20) {
        current_ticks = 0;
        current_ms++;
        count_40_micros_lost++;
        if (count_40_micros_lost == 25) {
            current_ms++;
            count_40_micros_lost = 0;
        }
    }

    process_sw_serial_tick();
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
        local_current = current_ms;
    }
    return local_current;
}

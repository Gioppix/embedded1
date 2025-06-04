// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=205
#include "analog.h"
#include "../serial/serial.h"

// Read ADCL (Low) and ADCH (High) in one go by defining ADCL as uint_16*
//
// We must read ADCL first, because after reading that the ADC is blocked from
// writing (guarantee of same conversion for both registers).
// Reading ADCH unlocks the ADC.
//
//   c0:	88 e7       	ldi	r24, 0x78	; 120
//   c2:	90 e0       	ldi	r25, 0x00	; 0
//   c4:	fc 01       	movw	r30, r24
//   c6:	80 81       	ld	r24, Z
//   c8:	91 81       	ldd	r25, Z+1	; 0x01
//
// The ASM shows that the "automatic" read of both is correct in order (at least
// in -O0)
//
//
// We could get 8-bit accuracy (which would be just fine) by setting
// ADLAR=1 and only readng ADCH, but the above explanation is cool so I'll keep
// it
#define ADCL EXPAND_ADDRESS_16(0x78)

#define ADMUX EXPAND_ADDRESS(0x7C)
BIT_NO(REFS0, 6);
BIT_NO(MUX0, 0);

#define ADCSRA EXPAND_ADDRESS(0x7A)
BIT_NO(ADEN, 7);
BIT_NO(ADIE, 3);
BIT_NO(ADPS0, 0);
#define PRESCALER 0b111

// REFS0..1
// Currently AVcc (5v)
#define REFERENCE 0b01

volatile boolean conversion_requested;
volatile boolean conversion_started;
volatile boolean conversion_complete;
volatile uint16_t result;

// ADC conversion complete
INTERRUPT(21) {
    result = ADCL;
    conversion_complete = true;

    // This check is only needed to detect inconsistencies; removing it poses no
    // harm
    if (!conversion_started) {
        throw_error(CONVERSION_NOT_STARTED);
    }
    conversion_started = false;

    // Disable ADC
    // This is needed because I found no other solution for single conversions
    CLEAR_BIT(ADCSRA, ADEN);
}

void init_ADC() {
    // Set reference
    ADMUX |= REFERENCE << REFS0;

    // Set prescaler
    ADCSRA |= PRESCALER << ADPS0;

    // Enable interrupts
    SET_BIT(ADCSRA, ADIE);
}

// Pins A0..5 in the Arduino Uno
void analog_read_pin_start(uint8_t pin_no) {
    conversion_requested = true;

    // Clear MUX0..3
    ADMUX &= 0xF0;

    // Better safe than sorry
    // We are discarding MUX3 as it's reserved bits + fixed voltages
    pin_no &= 0b111;

    // Sets MUX0..2 to the correct pin (channel)
    ADMUX |= pin_no << MUX0;

    conversion_complete = false;
    conversion_started = true;

    // Enable ADC
    SET_BIT(ADCSRA, ADEN);
}

// `analog_read_pin_start` must be called before this
uint16_t analog_read_pin_join() {
    if (!conversion_requested) {
        throw_error(CONVERSION_NOT_REQUESTED);
    }
    conversion_requested = false;

    // Wait for conversion
    while (!conversion_complete) {
        sleep();
    }

    return result;
}

uint16_t analog_read_pin_sync(uint8_t pin_no) {
    analog_read_pin_start(pin_no);
    return analog_read_pin_join();
}

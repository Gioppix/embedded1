#ifndef UTILS_HEADER
#define UTILS_HEADER

#include <stdint.h>

typedef uint8_t boolean;
#define true  1
#define false 0

#define SET_BIT(address, bit_n)         address |= 1 << bit_n
#define CLEAR_BIT(address, bit_n)       address &= ~(1 << bit_n)
#define MANAGE_BIT(address, bit_n, val) address = (address & ~(1 << bit_n)) | (val << bit_n)
#define GET_BIT(address, bit_n)         ((address >> bit_n) & 1)

#define EXPAND_ADDRESS_TYPE(address, type) *((volatile type *) (address))
#define EXPAND_ADDRESS(address)            EXPAND_ADDRESS_TYPE(address, uint8_t)
#define EXPAND_ADDRESS_16(address)         EXPAND_ADDRESS_TYPE(address, uint16_t)
#define BIT(name, num)                     static const uint8_t name = (1 << num)
#define BIT_NO(name, num)                  static const uint8_t name = num##U

#define INTERRUPT(n)                                                                               \
    void __attribute__((__signal__, __used__, __externally_visible__)) __vector_##n(void)

#define CRITICAL                                                                                   \
    for (boolean __critical_flag = (manage_global_interrupts(false), true); __critical_flag;       \
         __critical_flag         = false, manage_global_interrupts(true))

typedef enum {
    // Start from 2 so that can see led blink
    SEE_SERIAL = 2,
    USART_QUEUE_FULL,
    BAD_INTERRUPT,
    CONVERSION_NOT_STARTED,
    CONVERSION_NOT_REQUESTED,
    TWO_WIRES_ALREADY_SENDING,
    TWO_WIRES_NOT_SENDING,
    TWO_WIRES_NO_DATA_TO_SEND,
    TWO_WIRES_UNEXPECTED_STATE,
} ERROR;

void init_errors();
void throw_error(ERROR error_kind);

// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=54
#define SREG EXPAND_ADDRESS(0x5F)

// Inline since used in critical sections and must be short
__attribute__((always_inline)) inline void manage_global_interrupts(boolean enable) {
    MANAGE_BIT(SREG, 7, enable);
};

#define PORTB EXPAND_ADDRESS(0x25)
#define DEFINE_COLOR_FUNCTIONS(color, bit_no)                                                      \
    __attribute__((always_inline)) inline void on_##color() {                                      \
        SET_BIT(PORTB, bit_no);                                                                    \
    }                                                                                              \
    __attribute__((always_inline)) inline void off_##color() {                                     \
        CLEAR_BIT(PORTB, bit_no);                                                                  \
    }

DEFINE_COLOR_FUNCTIONS(blue, 0)
DEFINE_COLOR_FUNCTIONS(red, 1)
DEFINE_COLOR_FUNCTIONS(green, 2)

void init_blinks();
void short_blink();

void init_sleep();
void sleep();

void wait();

#endif

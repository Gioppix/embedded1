#ifndef UTILS_HEADER
#define UTILS_HEADER

#include <stdint.h>

typedef uint8_t boolean;
#define true 1
#define false 0

#define SET_BIT(address, bit_n) address |= 1 << bit_n
#define CLEAR_BIT(address, bit_n) address &= ~(1 << bit_n)
#define MANAGE_BIT(address, bit_n, val)                                        \
    address = (address & ~(1 << bit_n)) | (val << bit_n)
#define GET_BIT(address, bit_n) ((address >> bit_n) & 1)

#define EXPAND_ADDRESS_TYPE(address, type) *((volatile type *)(address))
#define EXPAND_ADDRESS(address) EXPAND_ADDRESS_TYPE(address, uint8_t)
#define EXPAND_ADDRESS_16(address) EXPAND_ADDRESS_TYPE(address, uint16_t)
#define BIT_NO(name, num) static const uint8_t name = num##U

#define INTERRUPT(n)                                                           \
    void __attribute__((__signal__, __used__,                                  \
                        __externally_visible__)) __vector_##n(void)

#define CRITICAL                                                               \
    for (boolean __critical_flag = (manage_global_interrupts(false), true);    \
         __critical_flag;                                                      \
         __critical_flag = false, manage_global_interrupts(true))

typedef enum {
    USART_QUEUE_FULL,
    BAD_INTERRUPT,
    CONVERSION_NOT_STARTED,
    CONVERSION_NOT_REQUESTED
} ERROR;

void init_errors();
void throw_error(ERROR error_kind);

// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=54
#define SREG EXPAND_ADDRESS(0x5F)

// Inline since used in critical sections and must be short
__attribute__((always_inline)) inline void
manage_global_interrupts(boolean enable) {
    MANAGE_BIT(SREG, 7, enable);
};

void init_short_blink();
void short_blink();

void init_sleep();
void sleep();

#endif

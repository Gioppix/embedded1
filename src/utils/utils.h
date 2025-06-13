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

// Fancy "hack" to let us use curly brackets. Thanks AI
#define CRITICAL                                                                                   \
    for (boolean __critical_flag = (manage_global_interrupts(false), true); __critical_flag;       \
         __critical_flag         = false, manage_global_interrupts(true))

// __attribute__((packed)) NEEDED here otherwise it defaults to 4 bytes...
typedef enum __attribute__((packed)) {
    // Must be 0 to easly check if error is false
    ALL_GOOD = 0,

    // Start from 2 so that we can see the led blink
    USART_ALREADY_SENDING = 2,
    LOSER,
    USART_IN_QUEUE_FULL,
    BAD_INTERRUPT,
    GAME_MAX_ENTITIES_REACHED,
    CONVERSION_NOT_STARTED,
    CONVERSION_NOT_REQUESTED,
    TWO_WIRES_NO_DATA_TO_SEND,
    TWO_WIRES_UNEXPECTED_STATE,
    TWO_WIRES_NO_START_ACK,
    TWO_WIRES_NO_DATA_ACK,
    TWO_WIRES_ARBITRATION_LOST,
    LCD_INVALID_ROW_OR_COL
} ERROR;

void init_errors();
void throw_error(ERROR error_kind);

// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=54
#define SREG EXPAND_ADDRESS(0x5F)

// Inline since used in critical sections and must be short
__attribute__((always_inline)) inline void manage_global_interrupts(boolean enable) {
    MANAGE_BIT(SREG, 7, enable);
};

#define MEASURE_TIME(var_name_param)                                                               \
    uint32_t var_name_param = 0;            /* Declare and initialize the duration variable */     \
    uint32_t __start_time_##var_name_param; /* Unique temporary variable for start time */         \
    /* This loop runs once: records start time, allows block execution, then calculates duration   \
     */                                                                                            \
    for (int __run_once_##var_name_param =                                                         \
             (__start_time_##var_name_param = get_current_time(), 1);                              \
         __run_once_##var_name_param;                                                              \
         __run_once_##var_name_param = 0,                                                          \
             var_name_param          = get_current_time() - __start_time_##var_name_param)
/* The user's code block becomes the body of this for-loop */


#define PORTB EXPAND_ADDRESS(0x25)
#define DEFINE_COLOR_FUNCTIONS(color, address, bit_no)                                             \
    __attribute__((always_inline)) inline void on_##color() {                                      \
        SET_BIT(address, bit_no);                                                                  \
    }                                                                                              \
    __attribute__((always_inline)) inline void off_##color() {                                     \
        CLEAR_BIT(address, bit_no);                                                                \
    }

DEFINE_COLOR_FUNCTIONS(blue, PORTB, 0)
DEFINE_COLOR_FUNCTIONS(red, PORTB, 1)
DEFINE_COLOR_FUNCTIONS(green, PORTB, 2)

void init_blinks();
void short_blink();

void init_sleep();
void sleep();

void wait();

#endif

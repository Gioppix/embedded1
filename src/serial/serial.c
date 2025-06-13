#include "serial.h"
#include "../gen_queue.h"
#include "../generated.h"
#include <stdint.h>

// TODO what?
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=149
#define FOSC 16000000 // Clock Speed
// Async double speed
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=146
#define MYUBRR (FOSC / 8 / BAUD - 1)

#define UBRR0H EXPAND_ADDRESS(0xC5)
#define UBRR0L EXPAND_ADDRESS(0xC4)

#define UCSR0C EXPAND_ADDRESS(0xC2)
BIT_NO(UCSZ00, 1);

#define UCSR0A EXPAND_ADDRESS(0xC0)

// This is a "strange" register: read and write ops are performed on different physical places
#define UDR0 EXPAND_ADDRESS(0xC6)
BIT_NO(U2X0, 1);

#define UCSR0B EXPAND_ADDRESS(0xC1)
BIT_NO(TXEN0, 3);
BIT_NO(RXEN0, 4);
BIT_NO(UDRIE0, 5);
BIT_NO(RXCIE0, 7);

DECLARE_QUEUE(usart_in, uint8_t, uint8_t, 10)

volatile boolean  sending                         = false;
volatile uint8_t *out_buffer                      = 0;
volatile uint16_t out_buffer_len                  = 0;
volatile uint16_t out_buffer_index_to_send        = 0;
volatile boolean (*generator_function)(uint8_t *) = 0;

// Interrupts (MUST DO N-1!!! THEY ARE ACTUALLY 0-BASED):
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=49

// USART, RX complete
INTERRUPT(18) {
    boolean res = usart_in_enqueue(UDR0);
    if (!res) {
        throw_error(USART_IN_QUEUE_FULL);
    }
}

// USART, Data Register Empty
INTERRUPT(19) {
    if (generator_function) {
        uint8_t data;
        boolean data_available = generator_function(&data);
        if (data_available) {
            UDR0 = data;
            return;
        }
    } else if (out_buffer_index_to_send < out_buffer_len) {
        UDR0 = out_buffer[out_buffer_index_to_send];
        out_buffer_index_to_send++;
        // UDRE interrupt remains enabled and will fire again
        // when UDR0 is ready for the next byte.
        return;
    }


    // Queue is empty, no more data to transmit. Disable interrupt
    CLEAR_BIT(UCSR0B, UDRIE0);
    sending = false;
}

void init_USART() {
    // Double speed
    SET_BIT(UCSR0A, U2X0);

    // Set baud rate, high and low (16 bits)
    UBRR0H = (unsigned char) (MYUBRR >> 8);
    UBRR0L = (unsigned char) MYUBRR;
    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    // Set frame format: 8data, 1 stop bit
    UCSR0C = (3 << UCSZ00);

    // Enable RX interrupts
    // SET_BIT(UCSR0B, RXCIE0);
}

void send_data(uint8_t *buffer, uint16_t len) {
    if (sending) {
        throw_error(USART_ALREADY_SENDING);
    }
    sending = true;

    generator_function       = 0;
    out_buffer               = buffer;
    out_buffer_len           = len;
    out_buffer_index_to_send = 0;


    SET_BIT(UCSR0B, UDRIE0);
}

void send_data_generator_f(volatile boolean f(uint8_t *)) {
    if (sending) {
        throw_error(USART_ALREADY_SENDING);
    }
    sending = true;

    generator_function = f;

    SET_BIT(UCSR0B, UDRIE0);
}

void serial_out_join() {
    while (sending) {
        sleep();
    }
}

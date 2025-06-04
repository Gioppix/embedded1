#include "serial.h"
#include "../gen_queue.h"
#include <stdint.h>

// TODO what?
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=149
#define FOSC 16000000 // Clock Speed
#define BAUD 9600
// Async double speed
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=146
#define MYUBRR (FOSC / 8 / BAUD - 1)

#define UBRR0H EXPAND_ADDRESS(0xC5)
#define UBRR0L EXPAND_ADDRESS(0xC4)
#define UCSR0C EXPAND_ADDRESS(0xC2)

BIT_NO(UCSZ00, 1);
#define UCSR0A EXPAND_ADDRESS(0xC0)
#define UDR0 EXPAND_ADDRESS(0xC6)
BIT_NO(U2X0, 1);

#define UCSR0B EXPAND_ADDRESS(0xC1)
BIT_NO(TXEN0, 3);
BIT_NO(RXEN0, 4);
BIT_NO(UDRIE0, 5);

DECLARE_QUEUE(usart_out, uint8_t, uint8_t, 20)

// Interrupts (MUST DO N-1!!! THEY ARE ACTUALLY 0-BASED):
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=49

// USART, data register empty
INTERRUPT(19) {
    uint8_t data_to_send;
    if (usart_out_dequeue(&data_to_send)) {
        // Data found in queue, send it
        UDR0 = data_to_send;
        // UDRE interrupt remains enabled and will fire again
        // when UDR0 is ready for the next byte.
    } else {
        // Queue is empty, no more data to transmit. Disable interrupt
        CLEAR_BIT(UCSR0B, UDRIE0);
    }
}

void init_USART() {
    // Double speed
    SET_BIT(UCSR0A, U2X0);

    // Set baud rate, high and low (16 bits)
    UBRR0H = (unsigned char)(MYUBRR >> 8);
    UBRR0L = (unsigned char)MYUBRR;
    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    // Set frame format: 8data, 1 stop bit
    UCSR0C = (3 << UCSZ00);
}

static boolean send_data(uint8_t *data, uint8_t len) {
    boolean success_enqueue;
    CRITICAL { success_enqueue = usart_out_enqueue_n(data, len); }

    if (success_enqueue) {
        // Enable the data empty interrupt
        // If already set this has no effect; otherwise start the interrupt
        // Dosen't need to be atomic since worst case scenario we get one more
        // interrupt
        SET_BIT(UCSR0B, UDRIE0);
    }

    return success_enqueue;
}

boolean println_char(uint8_t c) {
    uint8_t output[3];
    output[0] = c;
    output[1] = '\r';
    output[2] = '\n';

    return send_data(output, 3);
}

boolean println_num(uint32_t n) {
    // Max digits (3) + '\r' + '\n' + some extra (better safe than sorry)
    uint8_t output[20];
    uint8_t output_len = 0;

    if (n == 0) {
        output[output_len++] = '0';
    } else {
        uint32_t temp_n = n;
        uint8_t digit_count = 0;
        while (temp_n > 0) {
            digit_count++;
            temp_n /= 10;
        }

        output_len += digit_count;
        uint8_t pos = output_len - 1;
        while (n > 0) {
            output[pos--] = n % 10 + '0';
            n /= 10;
        }
    }
    output[output_len++] = '\r';
    output[output_len++] = '\n';

    return send_data(output, output_len);
}

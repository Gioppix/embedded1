#include "serial.h"
#include "../gen_queue.h"
#include <stdint.h>

// TODO what?
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=149
#define FOSC 16000000 // Clock Speed
#define BAUD 1000000
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

DECLARE_QUEUE(usart_out, uint8_t, uint8_t, 100)
DECLARE_QUEUE(usart_in, uint8_t, uint8_t, 100)

// Interrupts (MUST DO N-1!!! THEY ARE ACTUALLY 0-BASED):
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=49

// USART, RX complete
INTERRUPT(18) {
    boolean res = usart_in_enqueue(UDR0);
    if (!res) {
        throw_error(USART_IN_QUEUE_FULL);
    }
}

// USART, data register empty
INTERRUPT(19) {
    // UDR0 = 'a';
    uint8_t data_to_send = 'a';
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
    UBRR0H = (unsigned char) (MYUBRR >> 8);
    UBRR0L = (unsigned char) MYUBRR;
    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    // Set frame format: 8data, 1 stop bit
    UCSR0C = (3 << UCSZ00);

    // Enable RX interrupts
    SET_BIT(UCSR0B, RXCIE0);
}

static boolean send_data(uint8_t *data, uint8_t len) {
    boolean success_enqueue;
    CRITICAL {
        success_enqueue = usart_out_enqueue_n(data, len);
    }

    if (success_enqueue) {
        // Enable the data empty interrupt
        // If already set this has no effect; otherwise start the interrupt
        // Dosen't need to be atomic since worst case scenario we get one more
        // interrupt
        SET_BIT(UCSR0B, UDRIE0);
    }

    return success_enqueue;
}

void serial_queue_join() {
    volatile boolean queue_empty = false;

    while (!queue_empty) {
        CRITICAL {
            queue_empty = usart_out_empty();
        }
    }
}

boolean print_char(uint8_t c) {
    return send_data(&c, 1);
}

boolean print_num(uint32_t n) {
    // Max digits (10) + some extra (better safe than sorry)
    uint8_t output[20];
    uint8_t output_len = 0;

    if (n == 0) {
        output[output_len++] = '0';
    } else {
        uint32_t temp_n      = n;
        uint8_t  digit_count = 0;
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

    return send_data(output, output_len);
}

boolean print_bin(uint32_t n, uint8_t n_bits) {
    // 32 bits + some extra (better safe than sorry)
    uint8_t output[40];
    uint8_t output_len = 0;

    output_len += n_bits;
    uint8_t pos = output_len - 1;
    for (uint8_t i = 0; i < n_bits; i++) {
        output[pos--] = (n & 1) + '0';
        n >>= 1;
    }

    return send_data(output, output_len);
}

boolean print_str(const char *str) {
    // Calculate string length
    uint8_t str_len = 0;
    while (str[str_len] != '\0') {
        str_len++;
    }

    // Allocate buffer for string
    uint8_t output[str_len];
    uint8_t output_len = 0;

    // Copy string to output buffer
    for (uint8_t i = 0; i < str_len; i++) {
        output[output_len++] = str[i];
    }

    return send_data(output, output_len);
}

boolean print_ln() {
    uint8_t output[2];
    output[0] = '\r';
    output[1] = '\n';

    return send_data(output, 2);
}

boolean println_char(uint8_t c) {
    boolean success = print_char(c);
    if (success) {
        success = print_ln();
    }
    return success;
}

boolean println_num(uint32_t n) {
    boolean success = print_num(n);
    if (success) {
        success = print_ln();
    }
    return success;
}

boolean println_bin(uint32_t n, uint8_t n_bits) {
    boolean success = print_bin(n, n_bits);
    if (success) {
        success = print_ln();
    }
    return success;
}

boolean println_str(const char *str) {
    boolean success = print_str(str);
    if (success) {
        success = print_ln();
    }
    return success;
}

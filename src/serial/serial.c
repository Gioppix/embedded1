#include "serial.h"
#include "../gen_queue.h"
#include <stdint.h>

DECLARE_QUEUE(usart_out, uint8_t, uint8_t, 20)

// Interrupts (MUST DO N-1!!! THEY ARE 0-BASED):
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

void init_USART(unsigned int ubrr) {
    // Double speed
    SET_BIT(UCSR0A, U2X0);

    // Set baud rate, high and low (16 bits)
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    // Set frame format: 8data, 1 stop bit
    UCSR0C = (3 << UCSZ00);
}

inline void start_transmitting_USART() {
    // Enable the data empty interrupt
    // If already set this has no effect; otherwise start the interrupt
    // Dosen't need to be atomic since worst case scenario we get one more
    // interrupt
    SET_BIT(UCSR0B, UDRIE0);
}

boolean print_num(uint8_t n) {
    // Max digits for uint8_t (3) + '\r' + '\n' + some extra in case I forget
    // when I change to uint32_t
    uint8_t output[12];
    uint8_t output_len = 0;

    if (n == 0) {
        output[output_len++] = '0';
    } else {
        uint8_t temp_n = n;
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

    boolean success_enqueue;
    CRITICAL { success_enqueue = usart_out_enqueue_n(output, output_len); }

    if (success_enqueue) {
        start_transmitting_USART();
    }

    return success_enqueue;
}

// Page 72:
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf
#include <stdint.h>

// https://docs.arduino.cc/retired/hacking/software/PortManipulation/
// B (digital pin 8 to 13)
// C (analog input pins)
// D (digital pins 0 to 7)

#define EXPAND_ADDRESS(address) *((volatile uint8_t *)(address))
#define BIT_NO(name, num) uint8_t const name = num;

// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=59
#define PINB EXPAND_ADDRESS(0x23)
// 1=output, 0=input
#define DDRB EXPAND_ADDRESS(0x24)

// 1=high, 0=low
#define PORTB EXPAND_ADDRESS(0x25)

// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=54
#define SREG EXPAND_ADDRESS(0x5F)

#define PCICR EXPAND_ADDRESS(0x68)
// uint8_t *const EICRA = (uint8_t *)0x69;
#define PCMSK0 EXPAND_ADDRESS(0x6B)
#define PCMSK1 EXPAND_ADDRESS(0x6C)

#define BIT0 (1 << 0);
#define BIT1 (1 << 1);
#define BIT2 (1 << 2);
#define BIT3 (1 << 3);
#define BIT4 (1 << 4);
#define BIT5 (1 << 5);
#define BIT6 (1 << 6);
#define BIT7 (1 << 7);
#define SET_BIT(address, bit_n) address |= 1 << bit_n;
#define CLEAR_BIT(address, bit_n) address &= ~(1 << bit_n);

// TODO what?
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=149
#define FOSC 16000000 // Clock Speed
#define BAUD 9600
// Async double speed
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=146
#define MYUBRR FOSC / 8 / BAUD - 1

#define UBRR0H EXPAND_ADDRESS(0xC5)
#define UBRR0L EXPAND_ADDRESS(0xC4)
#define UCSR0B EXPAND_ADDRESS(0xC1)
#define UCSR0C EXPAND_ADDRESS(0xC2)
BIT_NO(RXEN0, 4)
BIT_NO(TXEN0, 3)
BIT_NO(USBS0, 3)
BIT_NO(UCSZ00, 1)
#define UCSR0A EXPAND_ADDRESS(0xC0)
#define UDR0 EXPAND_ADDRESS(0xC6)
BIT_NO(UDRE0, 5)
BIT_NO(U2X0, 1)
BIT_NO(UDRIE0, 5)

#define INTERRUPT(n)                                                           \
    void __attribute__((__signal__, __used__,                                  \
                        __externally_visible__)) __vector_##n(void)

void wait() {
    for (unsigned int i = 0; i < 30000; i++)
        for (int j = 0; j < 10; j++)
            ;
}

void short_wait() {
    for (int i = 0; i < 300; i++)
        ;
}

INTERRUPT(3) { PORTB |= (1 << 4); }

// USART, Rx complete
INTERRUPT(19){SET_BIT(PORTB, 1)}

// USART, data register empty
INTERRUPT(20) {}

// USART, Tx complete
INTERRUPT(21) { SET_BIT(PORTB, 1) }

void init_USART(unsigned int ubrr) {
    // - Check ongoing trasmissions
    // - Disable interrupts (if interrupt driven)
    // - Set baud rate
    // - Set frame format
    // - Enable TX/RX
    //

    // Double speed
    SET_BIT(UCSR0A, U2X0)

    // Set baud rate, high and low (16 bits)
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    // Set frame format: 8data, 1 stop bit
    UCSR0C = (3 << UCSZ00);

    // Enable data register empty interrupt
    SET_BIT(UCSR0B, UDRIE0)
}

void transmit_USART(unsigned char data) {
    // Wait for empty transmit buffer
    while (!(UCSR0A & (1 << UDRE0)))
        ;
    // Put data into buffer, sends the data
    UDR0 = data;
}

void print_num(uint32_t n) {
    if (n == 0) {
        transmit_USART('0');
    } else {
        char digits[10];
        int digit_count = 0;

        // Extract digits in reverse order
        while (n > 0) {
            digits[digit_count++] = n % 10 + '0';
            n /= 10;
        }

        // Print digits in correct order
        for (int i = digit_count - 1; i >= 0; i--) {
            transmit_USART(digits[i]);
        }
    }
    transmit_USART('\r');
    transmit_USART('\n');
}

int main(void) {
    // Show bootloop
    SET_BIT(DDRB, 1)
    SET_BIT(PORTB, 1)
    wait();
    CLEAR_BIT(PORTB, 1)

    init_USART(MYUBRR);

    // Enable global interrupts
    SET_BIT(SREG, 7)

    SET_BIT(DDRB, 0)

    uint8_t i = 0;
    while (1) {
        // SET_BIT(PORTB, 0)
        // wait();
        // CLEAR_BIT(PORTB, 0)
        // wait();
        print_num(i);
        // transmit_USART(i + 'a');
        i++;
        // i %= 26;
    }

    return 0;
}

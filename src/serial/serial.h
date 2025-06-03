#ifndef _SERIAL_H
#define _SERIAL_H

#include "../utils/utils.h"

// TODO what?
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=149
#define FOSC 16000000 // Clock Speed
#define BAUD 9600
// Async double speed
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=146
#define MYUBRR FOSC / 8 / BAUD - 1

#define UBRR0H EXPAND_ADDRESS(0xC5)
#define UBRR0L EXPAND_ADDRESS(0xC4)
#define UCSR0C EXPAND_ADDRESS(0xC2)

BIT_NO(USBS0, 3);
BIT_NO(UCSZ00, 1);
#define UCSR0A EXPAND_ADDRESS(0xC0)
#define UDR0 EXPAND_ADDRESS(0xC6)
BIT_NO(UDRE0, 5);
BIT_NO(U2X0, 1);

#define UCSR0B EXPAND_ADDRESS(0xC1)
BIT_NO(TXEN0, 3);
BIT_NO(RXEN0, 4);
BIT_NO(UDRIE0, 5);
BIT_NO(TXCIE0, 6);
BIT_NO(RXCIE0, 7);

BIT_NO(RXC0, 7);
BIT_NO(TXC0, 6);

void init_USART(unsigned int ubrr);
boolean print_num(uint8_t n);

#endif

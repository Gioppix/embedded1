#ifndef _PORTS_H
#define _PORTS_H

// https://docs.arduino.cc/retired/hacking/software/PortManipulation/
// B (digital pin 8 to 13)
// C (analog input pins)
// D (digital pins 0 to 7)

// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=72
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=59
#define PINB EXPAND_ADDRESS(0x23)
// 1=output, 0=input
#define DDRB EXPAND_ADDRESS(0x24)

// 1=high, 0=low
#define PORTB EXPAND_ADDRESS(0x25)

#endif

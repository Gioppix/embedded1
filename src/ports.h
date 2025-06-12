#ifndef _PORTS_H
#define _PORTS_H

// https://docs.arduino.cc/retired/hacking/software/PortManipulation/
// B (digital pin 8 to 13)
// C (analog input pins)
// D (digital pins 0 to 7)

// Each port pin consists of three register bits: DDxn, PORTxn, and PINxn. As shown in Section 13.4
// “Register Description” on page 72, the DDxn bits are accessed at the DDRx I/O address, the PORTxn
// bits at the PORTx I/O address, and the PINxn bits at the PINx I/O address. The DDxn bit in the
// DDRx register selects the direction of this pin. If DDxn is written logic one, Pxn is configured
// as an output pin. If DDxn is written logic zero, Pxn is configured as an input pin. If PORTxn is
// written logic one when the pin is configured as an input pin, the pull-up resistor is activated.
// To switch the pull-up resistor off, PORTxn has to be written logic zero or the pin has to be
// configured as an output pin. The port pins are tri-stated when reset condition becomes active,
// even if no clocks are running. If PORTxn is written logic one when the pin is configured as an
// output pin, the port pin is driven high (one). If PORTxn is written logic zero when the pin is
// configured as an output pin, the port pin is driven low (zero).
//
// Writing a logic one to PINxn toggles the value of PORTxn, independent on the value of DDRxn. Note
// that the SBI instruction can be used to toggle one single bit in a port.

// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=72
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=59
#define PINB EXPAND_ADDRESS(0x23)
// 1=output, 0=input
#define DDRB EXPAND_ADDRESS(0x24)


#define PIND  EXPAND_ADDRESS(0x29)
#define DDRD  EXPAND_ADDRESS(0x2A)
#define PORTD EXPAND_ADDRESS(0x2B)

#endif

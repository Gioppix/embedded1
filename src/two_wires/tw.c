// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=173
#include "tw.h"
#include "../serial/serial.h"
#include "../timers/timer.h"
#include <stdint.h>

// TWI Bit Rate Register
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=180
#define TWBR EXPAND_ADDRESS(0xB8)

// Default prescaler is 1, default TWBR is 0
// cpu_clock / ( 16 + 2 * TWBR * prescaler )
// 16000000 / ( 16 + 2 * 72 * 1 ) = 100khz (AI says it's guaranteed to work on
// I2C displays)
//
// For this freq stronger external pull-up resistors are required on SCA and SCL
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=264
#define TWBR_VALUE 72

// TWI Control Register
#define TWCR EXPAND_ADDRESS(0xBC)
// TWI hardware expects software action; clear manually after done
BIT(TWINT, 7);
// Start TWI as master. Must be cleared by software when the START condition has been transmitted
BIT(TWSTA, 5);
// When master, stop comms. Cleared automatically
BIT(TWSTO, 4);
// TWI ENable
BIT(TWEN, 2);
// TWI Interrupt Enable
BIT(TWIE, 0);

// TWI Status Register
#define TWSR EXPAND_ADDRESS(0xB9)
// Ignore prescaler bits
#define TWI_STATUS_MASK 0xF8
BIT_NO(TWPS0, 0);

// I2C Master Transmitter Mode Status Codes
#define TW_START_TRANSMITTED          0x08 // START condition transmitted
#define TW_REPEATED_START_TRANSMITTED 0x10 // Repeated START condition transmitted
#define TW_SLA_W_ACK_RECEIVED         0x18 // SLA+W transmitted, ACK received
#define TW_SLA_W_NACK_RECEIVED        0x20 // SLA+W transmitted, NOT ACK received
#define TW_DATA_ACK_RECEIVED          0x28 // Data byte transmitted, ACK received
#define TW_DATA_NACK_RECEIVED         0x30 // Data byte transmitted, NOT ACK received
#define TW_ARBITRATION_LOST           0x38 // Arbitration lost in SLA+W or data bytes

// TWI Data Register
#define TWDR EXPAND_ADDRESS(0xBB)

#define MAX_DATA_LEN 255

// Enable interrupts
#define DEFAULT_TWCR TWEN | TWIE

// To enable internal pull-up
#define PORTC    EXPAND_ADDRESS(0x28)
#define TW_PORTS ((1 << 4) | (1 << 5))

// No need for sending_data to be atomic since not modified inside interrupts
boolean sending_data;

volatile uint8_t slave_address;
volatile uint8_t data[MAX_DATA_LEN];
volatile uint8_t data_index_to_send;
volatile uint8_t data_len;
volatile boolean send_complete;
volatile ERROR   error;
volatile uint8_t retries_count;
#define MAX_RETIRES 10

// Return whether max_retires exeeded
// `maybe_can_continue_twcr`: If not 0, TWCR is set if MAX_RETIRES is not reached
boolean retry_or_error(ERROR maybe_err, uint8_t maybe_can_continue_twcr) {
    if (retries_count > MAX_RETIRES) {
        error         = maybe_err;
        send_complete = true;

        // REALLY REALLY REALLY important to always TWSTO (stop)
        // Hours wasted here count: 4
        TWCR = DEFAULT_TWCR | TWSTO | TWINT;
    }

    if (maybe_can_continue_twcr) {
        TWCR = maybe_can_continue_twcr;
    }

    retries_count++;
    return false;
}

void send_byte_and_continue() {
    if (data_index_to_send == data_len) {
        // No data, stop
        TWCR = DEFAULT_TWCR | TWSTO | TWINT;

        send_complete = true;
    } else {
        // Try to send byte
        TWDR = data[data_index_to_send];
        TWCR = DEFAULT_TWCR | TWINT;
    }
}

// 2-wire serial interface
INTERRUPT(24) {
    switch (TWSR & TWI_STATUS_MASK) {
        case TW_START_TRANSMITTED:
            // Handle repeated START condition transmitted
            // Load SLA+W or SLA+R, transmission depends on what is loaded
        case TW_REPEATED_START_TRANSMITTED:
            // Handle START condition transmitted
            // Load SLA+W, SLA+W will be transmitted; ACK or NOT ACK will be
            // received

            // Set Slave Address + Write flag (0)
            TWDR = slave_address << 1;

            // Clear TWINT to continue
            TWCR = DEFAULT_TWCR | TWINT;

            break;

        case TW_SLA_W_ACK_RECEIVED:
            // Handle SLA+W transmitted, ACK received
            // Load data byte or perform no TWDR action based on next operation

            send_byte_and_continue();

            break;

        case TW_SLA_W_NACK_RECEIVED: {
            // Handle SLA+W transmitted, NOT ACK received
            // Load data byte, repeated START, STOP, or STOP+START based on
            // requirements

            // Repeated start: retry (?)
            // Clear the flag to continue
            retry_or_error(TWO_WIRES_NO_START_ACK, DEFAULT_TWCR | TWINT | TWSTA);


            break;
        }

        case TW_DATA_ACK_RECEIVED:
            // Handle data byte transmitted, ACK received
            // Load next data byte, repeated START, STOP, or STOP+START based on
            // requirements

            data_index_to_send++;
            send_byte_and_continue();

            break;

        case TW_DATA_NACK_RECEIVED: {
            // Handle data byte transmitted, NOT ACK received
            // Load data byte, repeated START, STOP, or STOP+START based on
            // requirements

            boolean max_retries_reached = retry_or_error(TWO_WIRES_NO_DATA_ACK, 0);

            if (!max_retries_reached) {
                send_byte_and_continue();
            }

            break;
        }

        case TW_ARBITRATION_LOST: {
            // Handle arbitration lost in SLA+W or data bytes
            // Release bus and enter not addressed slave mode or transmit START when
            // bus becomes free

            // retry
            retry_or_error(TWO_WIRES_ARBITRATION_LOST, DEFAULT_TWCR | TWINT | TWSTA);


            break;
        }


        default:
            // Handle unexpected or unhandled status codes
            // Error handling or state machine reset

            throw_error(TWO_WIRES_UNEXPECTED_STATE);
            break;
    }
}

// Detail (master receiver)
// https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#page=183

void init_two_wires() {
    // Set Bit Rate
    TWBR = TWBR_VALUE;

    TWSR = 0;

    PORTC |= (1 << 4) | (1 << 5); // Enable internal pull-ups

    // Pullups are external
    // PORTC |= TW_PORTS;


    TWCR = DEFAULT_TWCR;
}


void write_two_wires_start(uint8_t local_slave_address,
                           uint8_t local_data[],
                           uint8_t local_data_len) {
    if (sending_data) {
        throw_error(TWO_WIRES_ALREADY_SENDING);
    }
    sending_data  = true;
    send_complete = false;
    error         = ALL_GOOD;
    retries_count = 0;

    slave_address      = local_slave_address;
    data_len           = local_data_len;
    data_index_to_send = 0;
    for (uint8_t i = 0; i < local_data_len; i++) {
        data[i] = local_data[i];
    }


    TWCR = DEFAULT_TWCR | TWSTA | TWINT;
}

ERROR write_two_wires_join() {
    if (!sending_data) {
        throw_error(TWO_WIRES_NOT_SENDING);
    }

    while (!send_complete) {
        sleep();
    }

    sending_data = false;

    return error;
}

ERROR
write_two_wires_sync(uint8_t local_slave_address, uint8_t local_data[], uint8_t local_data_len) {
    write_two_wires_start(local_slave_address, local_data, local_data_len);
    return write_two_wires_join();
}

void scan_i2c_addresses() {
    // println_str("Start scan");
    // serial_queue_join();

    char hex_addr_str[5]; // To store "0xHH\0" for printing
    hex_addr_str[0] = '0';
    hex_addr_str[1] = 'x';
    hex_addr_str[4] = '\0'; // Null-terminator

    for (uint8_t addr = 1; addr < 128; addr++) {
        uint8_t dummy_data = 0x00; // Data content is irrelevant for a 0-byte write check.

        if (write_two_wires_sync(addr, &dummy_data, 0) == ALL_GOOD) {
            uint8_t high_nibble = (addr >> 4) & 0x0F;
            uint8_t low_nibble  = addr & 0x0F;

            hex_addr_str[2] = (high_nibble < 10) ? (high_nibble + '0') : (high_nibble - 10 + 'A');
            hex_addr_str[3] = (low_nibble < 10) ? (low_nibble + '0') : (low_nibble - 10 + 'A');

            // println_str(hex_addr_str);

            // serial_queue_join();
        }
        sleep_ms(10);
    }

    // println_str("Scan done");
    // serial_queue_join();
}

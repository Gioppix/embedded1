#ifndef _SERIAL_H
#define _SERIAL_H

#include "../utils/utils.h"

#define SET_COMMAND(x) (x | 1 << 7)
#define SET_DATA(x)    (x & ~(1 << 7))

void init_USART();
void send_data(uint8_t *, uint16_t);
void send_data_generator_f(volatile boolean (*)(uint8_t *));

// Wait for empty queue
void serial_out_join();

#endif

#ifndef _SERIAL_H
#define _SERIAL_H

#include "../utils/utils.h"

void    init_USART();
boolean println_num(uint32_t n);
boolean println_char(uint8_t charr);
boolean println_str(const char *);
boolean println_bin(uint32_t n, uint8_t n_bits);

// Wait for empty queue
void serial_queue_join();

#endif

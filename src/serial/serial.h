#ifndef _SERIAL_H
#define _SERIAL_H

#include "../utils/utils.h"

void    init_USART();
boolean print_num(uint32_t n);
boolean print_char(uint8_t charr);
boolean print_str(const char *);
boolean print_bin(uint32_t n, uint8_t n_bits);

boolean print_ln();
boolean println_num(uint32_t n);
boolean println_char(uint8_t charr);
boolean println_str(const char *);
boolean println_bin(uint32_t n, uint8_t n_bits);

// Wait for empty queue
void serial_queue_join();

#endif

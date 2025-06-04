#ifndef _SERIAL_H
#define _SERIAL_H

#include "../utils/utils.h"

void init_USART();
boolean println_num(uint32_t n);
boolean println_char(uint8_t charr);

#endif

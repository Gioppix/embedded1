#ifndef _SW_SERIAL_H
#define _SW_SERIAL_H

#include "../utils/utils.h"
#include <stdint.h>

void init_sw_serial();

void process_sw_serial_tick();

boolean send_byte(uint8_t);

volatile extern uint8_t deb;

#endif

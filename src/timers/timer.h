#ifndef _TIMER_H
#define _TIMER_H

#include "../utils/utils.h"
#include <stdint.h>

void init_timer0();

uint32_t get_current_time();

void sleep(uint32_t ms);

#endif

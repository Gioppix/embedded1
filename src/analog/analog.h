#ifndef _ANALOG_H
#define _ANALOG_H

#include "../utils/utils.h"

// Separate start and join so that immediately waiting for the ADC is optional

void analog_read_pin_start(uint8_t pin_no);
uint16_t analog_read_pin_join();

uint16_t analog_read_pin_sync(uint8_t pin_no);
void init_ADC();

#endif

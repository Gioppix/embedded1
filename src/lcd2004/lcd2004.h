#ifndef _LCD2004_H
#define _LCD2004_H

#include "../two_wires/tw.h"
#include "../utils/utils.h"

void init_lcd_2004();

ERROR lcd_write_string(const char *);
ERROR lcd_write_uint16(uint16_t);

ERROR lcd_clean();

ERROR lcd_set_cursor(uint8_t row, uint8_t col);

void throw_error_if_present(ERROR);

#endif

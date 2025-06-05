#ifndef _LCD2004_H
#define _LCD2004_H

#include "../two_wires/tw.h"
#include "../utils/utils.h"

void init_lcd_2004();

TWO_WIRES_ERR lcd_write_string(const char *);

TWO_WIRES_ERR lcd_clean();

TWO_WIRES_ERR lcd_set_cursor(uint8_t row, uint8_t col);

void throw_error_if_present(TWO_WIRES_ERR);

#endif

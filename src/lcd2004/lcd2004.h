#ifndef _LCD2004_H
#define _LCD2004_H

#include "../two_wires/tw.h"
#include "../utils/utils.h"

void init_lcd_2004();

void lcd_write_string(const char *);
void lcd_write_uint16(uint16_t);

void lcd_clean();

void lcd_set_cursor(uint8_t row, uint8_t col);


#endif

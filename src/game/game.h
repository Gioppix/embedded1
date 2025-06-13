#ifndef GAME_H
#define GAME_H

#include "../utils/utils.h"
#include <stdint.h>

extern uint8_t score;
extern uint8_t bullets;

void init_game();
// angle between 0 and PI, in radians
void process_tick(uint32_t, float, boolean);

void start_sending_frame();


#endif

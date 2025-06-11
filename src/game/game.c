#include <stdint.h>
#define BITS_PER_COLOR 2
#define COLORS_PER_BIT (8 / BITS_PER_COLOR)
#define SCREENX        (COLORS_PER_BIT * 12)
#define SCREENY        SCREENX

typedef uint8_t canvas_t[SCREENX / COLORS_PER_BIT][SCREENY];
// Never used; useful to check the size
const uint16_t ARRAY_SIZE = sizeof(canvas_t);

canvas_t canvas1;
canvas_t canvas2;

canvas_t *current_canvas = &canvas1;


// Colors store example:
//   x0  x1  x2  x3  x4  x5  x6  x7
//
//   1   2   3   4   1   2   3   4   y0
//   5   6   7   8   5   6   7   8   y1
//   9  10  11  12   9  10  11  12   y2
//
// 1.2.3.4     1.2.3.4
// 5.6.7.8     5.6.7.8
// 9.10.11.12  9.10.11.12

void render(canvas_t *canvas_to_use) {
    uint8_t i = 0;
    for (uint8_t x = 0; x < SCREENX; x++)
        for (uint8_t y = 0; y < SCREENX; y++) {
            if (x % COLORS_PER_BIT == 0) {
                // Clean prev frame, once per byte
                *canvas_to_use[x / COLORS_PER_BIT][y] = 0;
            }

            uint8_t shifted_color = i << (BITS_PER_COLOR * (x % COLORS_PER_BIT));
            *canvas_to_use[x / COLORS_PER_BIT][y] |= shifted_color;
            i %= 4;
        }
}

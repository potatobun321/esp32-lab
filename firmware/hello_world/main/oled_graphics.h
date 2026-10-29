#ifndef OLED_GRAPHICS_H
#define OLED_GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

#define OLED_WIDTH       128
#define OLED_HEIGHT      64
#define OLED_BUFFER_SIZE (OLED_WIDTH * OLED_HEIGHT / 8)

typedef enum {
    COLOR_BLACK  = 0,
    COLOR_WHITE  = 1,
    COLOR_INVERT = 2
} oled_color_t;

void oled_clear(oled_color_t color);
void oled_set_pixel(int x, int y, oled_color_t color);
uint8_t oled_get_pixel(int x, int y);
void oled_draw_line(int x0, int y0, int x1, int y1, oled_color_t color);
void oled_draw_rect(int x, int y, int w, int h, oled_color_t color);
void oled_fill_rect(int x, int y, int w, int h, oled_color_t color);
void oled_draw_circle(int cx, int cy, int r, oled_color_t color);
void oled_fill_circle(int cx, int cy, int r, oled_color_t color);
void oled_draw_char(int x, int y, char c, oled_color_t color);
void oled_draw_text(int x, int y, const char *text, oled_color_t color);
void oled_draw_bitmap(int x, int y, int w, int h, const uint8_t *bitmap, oled_color_t color);
uint8_t* oled_get_framebuffer(void);

#endif // OLED_GRAPHICS_H

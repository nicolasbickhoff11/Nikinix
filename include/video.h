#include <stdint.h>

#define RGB(r, g, b) ((uint32_t)(((uint8_t)(r)|((uint16_t)((uint8_t)(g))<<8))|(((uint32_t)(uint8_t)(b))<<16)))

void get_fb();
void get_pitch();
void put_pixel_buff(uint32_t* buff, int x, int y, int w, int h, uint32_t color);

void draw_rectangle(int x, int y, int w, int h, uint32_t color);
void draw_circle(int w, int h, int cx, int cy, int radius, uint32_t color);
void draw_line(int x0, int y0, int x1, int y1, int w, int h, uint32_t color);
void blit(uint32_t* src, int dstx, int dsty, int w, int h);

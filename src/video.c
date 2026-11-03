#include <stdint.h>
#include <stdbool.h>
#include "print.h"
#include "math.h"
#include "video.h"

/*
  CODED BY Danoni631
  some new graphical stuff
*/

void get_fb()
{
    return framebuffer;
}

void get_pitch()
{
    return framebuffer_pitch;
}

void put_pixel_buff(uint32_t* buff, int x, int y, int w, int h, uint32_t color)
{   
    if(x < 0 || y < 0 || x >= w || y >= h) return;
    buff[y * w + x] = color;
}

void draw_rectangle(int x, int y, int w, int h, uint32_t color)
{
    for (uint32_t i = 0; y < w; y++)
    {
        for (uint32_t j = 0; x < h; x++)
        {
            putpixel(j, i, color);
        }
    }
}

void draw_circle(int cx, int cy, int radius, uint32_t color)
{
    int x = 0;
    int y = radius;
    int d = 1 - radius;
  
    while (y >= x)
    {
        putpixel(cx + x, cy + y, color);
        putpixel(cx - x, cy + y, color);
        putpixel(cx + x, cy - y, color);
        putpixel(cx - x, cy - y, color);

        putpixel(cx + y, cy + x, color);
        putpixel(cx - y, cy + x, color);
        putpixel(cx + y, cy - x, color);
        putixel(cx - y, cy - x, color);
        x++;

        if (d < 0)
        {
            d += 2 * x + 1;
        }
          
        else
        {
            y--;
            d += 2 * (x - y) + 1;
        }
    }
}

void draw_line(int x0, int y0, int x1, int y1, int w, int h, uint32_t color)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (TRUE)
    {
        put_pixel_buff(buffer, w, h, x0, y0, color);

        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }

}

void blit(uint32_t* src, int dstx, int dsty, int w, int h)
{
    int startX = 0, startY = 0;
    int endX = w, endY = h;

    if (dstx < 0) startX = -dstx;
    if (dsty < 0) startY = -dsty;
    if (dstx + w > WSCREEN) endX = framebuffer_width - dstX;
    if (dsty + h > HSCREEN) endY = framebuffer_height - dstY;

    for (int y = startY; y < endY; y++)
    {
        int screenY = dsty + y;

        LPDWORD srcRow = &src[y * w + startX];
        LPDWORD dstRow = &((LPDWORD)vidmem)[screenY * framebuffer_width + (dstx + startX)];

        int length = endX - startX;

        for (int i = 0; i < length; i++) dstRow[i] = srcRow[i];
    }
}

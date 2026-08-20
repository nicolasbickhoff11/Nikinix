#include "string.h"
#include "print.h"
#include "io.h"
#include <stdarg.h>
#include <stdint.h>

#define PORT 0x3f8

void init_serial() {
    outb(PORT + 1, 0x00);    // Disable all interrupts
    outb(PORT + 3, 0x80);    // Enable DLAB
    outb(PORT + 0, 0x03);    // Divisor lo byte (38400 baud)
    outb(PORT + 1, 0x00);    // Divisor hi byte
    outb(PORT + 3, 0x03);    // 8 bits, no parity, 1 stop bit
    outb(PORT + 2, 0xC7);    // Enable FIFO, clear
    outb(PORT + 4, 0x0B);    // Enable IRQs, RTS/DSR
}

int com1_putc(char c) {
    while ((inb(PORT + 5) & 0x20) == 0);
    outb(PORT, c);
    return 1;
}

int com1_puts(const char *s) {
    int count = 0;
    if (!s) return 0;
    while (*s) {
        com1_putc(*s++);
        count++;
    }
    return count;
}

void putpixel(int x, int y, uint32_t color) {
    uint32_t *fb = (uint32_t*)framebuffer;

    fb[y * (framebuffer_pitch / 4) + x] = color;
}

#include "font.h"

void draw_char(int x, int y, char c, uint32_t fg, uint32_t bg)
{
    uint8_t *glyph = (uint8_t*)&font[(uint8_t)c * 16];

    for (int row = 0; row < 16; row++) {
        uint8_t line = glyph[row];

        for (int col = 0; col < 8; col++) {
            if (line & (1 << (7 - col))) {
                putpixel(x + col, y + row, fg);
            } else {
                putpixel(x + col, y + row, bg);
            }
        }
    }
}

void draw_string(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    while (*s) {
        draw_char(x, y, *s, fg, bg);
        x += 8;
        s++;
    }
}

int cursor_x = 0;
int cursor_y = 0;

void clear_video() {
    for (uint32_t y = 0; y < framebuffer_height; y++) {
        for (uint32_t x = 0; x < framebuffer_width; x++) {
            putpixel(x, y, 0x00000000);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

int fb_print_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += 16;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\0') {
        asm volatile ("nop");
    } else {
        draw_char(cursor_x, cursor_y, c, 0xFFFFFF, 0x000000);
        cursor_x += 8;
    }

    if ((uint32_t)cursor_x >= framebuffer_width) {
        cursor_x = 0;
        cursor_y += 16;
    }

    if ((uint32_t)cursor_y + 16 > framebuffer_height) {
        fb_scroll();
    }

    return 1;
}

void fb_scroll() {
    uint32_t *fb = (uint32_t*)framebuffer;  // garante que é ponteiro
    uint32_t row_pixels = framebuffer_pitch / 4; // cada pixel 4 bytes
    uint32_t scroll_lines = 16; // altura do caractere

    // move tudo para cima
    memmove(
        fb,
        fb + row_pixels * scroll_lines,
        (framebuffer_height - scroll_lines) * row_pixels * sizeof(uint32_t)
    );

    // limpa a última linha
    memset(
        fb + row_pixels * (framebuffer_height - scroll_lines),
        0,
        scroll_lines * row_pixels * sizeof(uint32_t)
    );

    cursor_y -= scroll_lines;
}

int fb_print(const char *s) {
    int total = 0;
    while (*s) { if (*s == '\x1B' && *(s+1) == 'c') { clear_video(); total += 2; s += 2; continue; } fb_print_char(*s++); total++; }
    return total;
}

int print_char(char c) {
    fb_print_char(c);
    com1_putc(c);
    return 1;
}

int print(const char *s) {
    int total = 0;
    while (s[total]) total++;
    fb_print(s);
    com1_puts(s);
    return total;
}



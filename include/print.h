// print.h
#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>
#include "printf.h"

#define kprintf printf

extern int cursor_x;
extern int cursor_y;
extern uint64_t framebuffer;
extern uint32_t framebuffer_pitch;
extern uint32_t framebuffer_width;
extern uint32_t framebuffer_height;
extern uint8_t  framebuffer_bpp;
extern char     last_char;
//extern uint8_t  framebuffer_type;

void fb_scroll();
void init_serial(void);
int com1_putc(char c);
int com1_puts(const char *s);
void putpixel(int x, int y, uint32_t color);
void draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void draw_string(int x, int y, const char *s, uint32_t fg, uint32_t bg);
void clear_video();
int fb_print_char(char c);
int fb_print(const char *s);
int print_char(char c);
int print(const char *s);
int print_hex(uintptr_t ptr);
int print_hex32(uint32_t n);
int print_hex64(uint64_t n);
int print_pointer(uintptr_t ptr);
int print_dec(int n);
int print_uint(uint32_t n);
int kprintf(const char *fmt, ...);

#endif

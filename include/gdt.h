#ifndef _GDT_H
#define _GDT_H 1

#include <stdint.h>

struct gdt_segment {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access_byte;
    uint8_t  limit_high_and_flags;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_descriptor {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed));

extern struct gdt_segment    gdt[7];
extern struct gdt_descriptor gdtr;

void setup_gdt();

#endif

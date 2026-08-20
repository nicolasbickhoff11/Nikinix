#ifndef _TSS_H
#define _TSS_H 1

#include <stdint.h>

struct tss_entry_struct {
    uint32_t reserved0;
    uint64_t rsp0;      // Stack pointer loaded on privilege change to Ring 0
    uint64_t rsp1;      // Stack pointer loaded on privilege change to Ring 1
    uint64_t rsp2;      // Stack pointer loaded on privilege change to Ring 2
    uint64_t reserved1;
    uint64_t ist[7];    // Interrupt Stack Table (IST1 to IST7)
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base; // Offset from TSS base to the I/O Permission Bitmap
} __attribute__((packed));

extern struct tss_entry_struct tss;

void setup_tss();

#endif
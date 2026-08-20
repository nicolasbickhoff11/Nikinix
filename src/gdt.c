#include <stdint.h>
#include "gdt.h"
#include "tss.h"

struct gdt_segment    gdt[7] = { 0 };
struct gdt_descriptor gdtr = { 0 };

void setup_gdt() {
    gdt[0].limit_low            = 0x0000;
    gdt[0].base_low             = 0x0000;
    gdt[0].base_mid             = 0x00;
    gdt[0].access_byte          = 0x00;
    gdt[0].limit_high_and_flags = 0x00;
    gdt[0].base_high            = 0x00;
    gdt[1].limit_low            = 0xFFFF;
    gdt[1].base_low             = 0x0000;
    gdt[1].base_mid             = 0x00;
    gdt[1].access_byte          = 0x9A;
    gdt[1].limit_high_and_flags = 0xAF;
    gdt[1].base_high            = 0x00;
    gdt[2].limit_low            = 0xFFFF;
    gdt[2].base_low             = 0x0000;
    gdt[2].base_mid             = 0x00;
    gdt[2].access_byte          = 0x92;
    gdt[2].limit_high_and_flags = 0xCF;
    gdt[2].base_high            = 0x00;
    gdt[3].limit_low            = 0xFFFF;
    gdt[3].base_low             = 0x0000;
    gdt[3].base_mid             = 0x00;
    gdt[3].access_byte          = 0xFA;
    gdt[3].limit_high_and_flags = 0xAF;
    gdt[3].base_high            = 0x00;
    gdt[4].limit_low            = 0xFFFF;
    gdt[4].base_low             = 0x0000;
    gdt[4].base_mid             = 0x00;
    gdt[4].access_byte          = 0xF2;
    gdt[4].limit_high_and_flags = 0xCF;
    gdt[4].base_high            = 0x00;
    uint32_t tss_limit = sizeof(tss) - 1;
    gdt[5].limit_low            = tss_limit & 0xFFFF;
    gdt[5].base_low             = (uint64_t)&tss & 0xFFFF;
    gdt[5].base_mid             = ((uint64_t)&tss >> 16) & 0xFF;
    gdt[5].access_byte          = 0x89;
    gdt[5].limit_high_and_flags = (tss_limit >> 16) & 0x0F;
    gdt[5].base_high            = ((uint64_t)&tss >> 24) & 0xFF;
    *((uint32_t*)&gdt[6]) = ((uint64_t)&tss >> 32) & 0xFFFFFFFF;

    gdtr.size   = sizeof(gdt) - 1;
    gdtr.offset = (uint64_t)gdt;
}

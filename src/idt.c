#include <stdint.h>
#include "idt.h"

struct interrupt_descriptor idt[256] = { 0 };
struct idtr idtr = { 0 };

void set_idt_entry(uint64_t offset, uint8_t num, uint16_t selector, uint8_t ist, uint8_t type_attr) {
    struct interrupt_descriptor *entry = &idt[num];
    entry->offset_1 = (uint16_t)(offset & 0xFFFF);
    entry->selector = selector;
    entry->ist = ist;
    entry->type_attributes = type_attr;
    entry->offset_2 = (uint16_t)((offset >> 16) & 0xFFFF);
    entry->offset_3 = (uint32_t)((offset >> 32) & 0xFFFFFFFF);
    entry->zero = 0;
}

void load_idt() {
    idtr.size = sizeof(idt) - 1;
    idtr.offset = (uint64_t)idt;
    asm volatile ("lidt %0" :: "m"(idtr));
}

#include <stdint.h>
#include "io.h"
#include "pic.h"

void pic_remap() {
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();
    outb(PIC1_DATA, 1 << CASCADE_IRQ);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void set_irq_mask(uint8_t irqline) {
    uint16_t port;
    uint8_t value;

    if (irqline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irqline -= 8;
    }
    value = inb(port) | (1 << irqline);
    outb(port, value);
}

void clear_irq_mask(uint8_t irqline) {
    uint16_t port;
    uint8_t value;

    if (irqline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irqline -= 8;
    }
    value = inb(port) & ~(1 << irqline);
    outb(port, value);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);

    outb(PIC1_COMMAND, PIC_EOI);
}

// AI code

uint8_t pic_read_irqs() {
    outb(0x20, 0x0A);
    return inb(0x20);
}

uint8_t pic_read_isr() {
    outb(0x20, 0x0B);
    return inb(0x20);
}

uint8_t pic_read_imr() {
    return inb(0x21);
}

void disable_pic() {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

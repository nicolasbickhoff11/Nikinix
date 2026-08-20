#include "isr.h"
#include "print.h"
#include <stdint.h>

void general_fault_handler(struct interrupt_frame_with_error_code *frame) {
    int ext = frame->error_code & 0x1;
    int table = (frame->error_code >> 1) & 0x3;
    int index = (frame->error_code >> 3) & 0x1FFF;

    kprintf("[ERROR] #GP: General Protection Fault!\n");
    kprintf("[DEBUG] RIP: 0x%lx\n", frame->rip);
    kprintf("[DEBUG] ERROR CODE: 0x%lx\n", frame->error_code);
    kprintf("[DEBUG] REGS: { RAX=0x%lx, RBX=0x%lx, RCX=0x%lx, RDX=0x%lx, RSI=0x%lx, RDI=0x%lx, RBP=0x%lx, R8=0x%lx, R9=0x%lx, R10=0x%lx, R11=0x%lx, R12=0x%lx, R13=0x%lx, R14=0x%lx, R15=0x%lx}\n", frame->rax, frame->rbx, frame->rcx, frame->rdx, frame->rsi, frame->rdi, frame->rbp, frame->r8, frame->r9, frame->r10, frame->r11, frame->r12, frame->r13, frame->r14, frame->r15);

    const char *t = 0;
    switch (table) {
        case 0: t = "GDT"; break;
        case 1: t = "IDT"; break;
        case 2: t = "LDT"; break;
        default: t = "INVALID"; break;
    }

    kprintf("[CAUSE] %s selector fault in index 0x%x (EXT=%d)\n", t, index, ext);
    asm volatile ("cli; hlt");
}

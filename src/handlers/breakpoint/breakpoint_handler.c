#include <stdint.h>
#include "print.h"
#include "isr.h"

void breakpoint_handler(struct interrupt_frame *frame) {
    kprintf("[DEBUG] #BP: Breakpoint\n");
    kprintf("[DEBUG] RIP: 0x%lx\n", frame->rip);
    kprintf("[DEBUG] REGS: { RAX=0x%lx, RBX=0x%lx, RCX=0x%lx, RDX=0x%lx, RSI=0x%lx, RDI=0x%lx, RBP=0x%lx, R8=0x%lx, R9=0x%lx, R10=0x%lx, R11=0x%lx, R12=0x%lx, R13=0x%lx, R14=0x%lx, R15=0x%lx}\n", frame->rax, frame->rbx, frame->rcx, frame->rdx, frame->rsi, frame->rdi, frame->rbp, frame->r8, frame->r9, frame->r10, frame->r11, frame->r12, frame->r13, frame->r14, frame->r15);
}
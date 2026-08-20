#include "print.h"
#include "isr.h"
#include <stdint.h>

void page_fault_handler(struct interrupt_frame_with_error_code *frame) {
    kprintf("[ERROR] #PF: Page Fault!\n");
    kprintf("[DEBUG] ERROR CODE: 0x%lx\n", frame->error_code);
    uint64_t cr2 = 0;
    asm volatile ("mov %%cr2, %0" : "=r"(cr2));
    kprintf("[DEBUG] CR2: 0x%lx\n", cr2);
    kprintf("[DEBUG] RIP: 0x%lx\n", frame->rip);
    kprintf("[DEBUG] REGS: { RAX=0x%lx, RBX=0x%lx, RCX=0x%lx, RDX=0x%lx, RSI=0x%lx, RDI=0x%lx, RBP=0x%lx, R8=0x%lx, R9=0x%lx, R10=0x%lx, R11=0x%lx, R12=0x%lx, R13=0x%lx, R14=0x%lx, R15=0x%lx}\n", frame->rax, frame->rbx, frame->rcx, frame->rdx, frame->rsi, frame->rdi, frame->rbp, frame->r8, frame->r9, frame->r10, frame->r11, frame->r12, frame->r13, frame->r14, frame->r15);
    if (!(frame->error_code & (1 << 0)) && frame->error_code & (1 << 1)) {
        kprintf("[CAUSE] trying to write at a page non-present in virtual address 0x%lx!\n", cr2);
    }
    asm volatile ("cli; hlt");
}

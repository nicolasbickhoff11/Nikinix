#include "syscall/regs.h"
#include "print.h"
#include <stdint.h>
#include <stddef.h>

long (*syscall_table[256])(struct user_regs *regs) = {0};
extern char last_char;

typedef long ssize_t;

ssize_t sys_write(int fd, const void *buf, size_t size) {
    if (fd == 0 || fd == 1 || fd == 2) {
        size_t counter = 0;
        while (counter != size) { kprintf("%c", ((char*)buf)[counter++]); }
        return counter;
    }
    return -9;
}

ssize_t sys_read(int fd, void *buf, size_t size) {
    if (fd == 0) {
        size_t counter = 0;
        while (counter != size) {
            asm volatile ("sti; hlt");
            if (last_char == '\n') return counter;
            if (last_char == '\0') continue;
            ((char*)buf)[counter++] = last_char;
            kprintf("%c", last_char);
            last_char = '\0';
        }
    }
    return -9;
}

extern uint64_t timer_ticks;

int sys_sleep(int seconds) {
    if (!seconds) return 1;
    uint64_t future_tick = timer_ticks + 100 * seconds;
    while (timer_ticks < future_tick) {
        asm volatile ("hlt");
    }
    return 0;
}

long _x64_sys_sleep(struct user_regs *regs) {
    return sys_sleep(regs->rdi);
}

long _x64_sys_write(struct user_regs *regs) {
    return sys_write(regs->rdi, regs->rsi, regs->rdx);
}

long _x64_sys_read(struct user_regs *regs) {
    return sys_read(regs->rdi, regs->rsi, regs->rdx);
}

void syscall_main(struct user_regs *regs) {

    uint64_t num = regs->rax;
    if (!syscall_table[num]) {
        regs->rax = -38; // ENOSYS
        return;
    } else {
        regs->rax = syscall_table[num](regs);
        return;
    }
}
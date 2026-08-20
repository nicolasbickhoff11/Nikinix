#ifndef _SYSCALL_ENABLE_H
#define _SYSCALL_ENABLE_H 1

extern long (*syscall_table[256])(struct user_regs *regs);
void setup_syscall();

#endif
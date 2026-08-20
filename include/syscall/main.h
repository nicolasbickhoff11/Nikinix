#ifndef _SYSCALL_MAIN_H
#define _SYSCALL_MAIN_H 1

#include "syscall/regs.h"

#define _NR_write 0
#define _NR_read 1
#define _NR_sleep 2

long _x64_sys_write(struct user_regs *regs);
long _x64_sys_read(struct user_regs *regs);
long _x64_sys_sleep(struct user_regs *regs);

#endif
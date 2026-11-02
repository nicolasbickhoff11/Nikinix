#ifndef _SYSCALL_MAIN_H
#define _SYSCALL_MAIN_H 1

#include "syscall/regs.h"

#define _NR_write 0
#define _NR_read 1
#define _NR_sleep 2
#define _NR_uname 3
#define _NR_getdirentries 4
#define _NR_mkdir 5
#define _NR_create 6
#define _NR_remove 7
#define _NR_sethostname 8
#define _NR_getcwd 9
#define _NR_chdir 10
#define _NR_open 11
#define _NR_close 12

long _x64_sys_write(struct user_regs *regs);
long _x64_sys_read(struct user_regs *regs);
long _x64_sys_sleep(struct user_regs *regs);
long _x64_sys_uname(struct user_regs *regs);
long _x64_sys_getdirentries(struct user_regs *regs);
long _x64_sys_mkdir(struct user_regs *regs);
long _x64_sys_create(struct user_regs *regs);
long _x64_sys_remove(struct user_regs *regs);
long _x64_sys_sethostname(struct user_regs *regs);
long _x64_sys_getcwd(struct user_regs *regs);
long _x64_sys_chdir(struct user_regs *regs);
long _x64_sys_open(struct user_regs *regs);
long _x64_sys_close(struct user_regs *regs);

#endif
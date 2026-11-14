#ifndef _SYSCALL_MAIN_H
#define _SYSCALL_MAIN_H 1

#include "syscall/regs.h"

#define _NR_write 0
#define _NR_read 1
#define _NR_sleep 2
#define _NR_uname 3
#define _NR_getdirentries 4
#define _NR_mkdir 5
#define _NR_creat 6
#define _NR_rmdir 7
#define _NR_sethostname 8
#define _NR_getcwd 9
#define _NR_chdir 10
#define _NR_open 11
#define _NR_close 12
#define _NR_unlink 13
#define _NR_fork 14
#define _NR_waitpid 15
#define _NR_exit 16
#define _NR_getpid 17

long _x64_sys_write(struct user_regs *regs);
long _x64_sys_read(struct user_regs *regs);
long _x64_sys_sleep(struct user_regs *regs);
long _x64_sys_uname(struct user_regs *regs);
long _x64_sys_getdirentries(struct user_regs *regs);
long _x64_sys_mkdir(struct user_regs *regs);
long _x64_sys_creat(struct user_regs *regs);
long _x64_sys_rmdir(struct user_regs *regs);
long _x64_sys_sethostname(struct user_regs *regs);
long _x64_sys_getcwd(struct user_regs *regs);
long _x64_sys_chdir(struct user_regs *regs);
long _x64_sys_open(struct user_regs *regs);
long _x64_sys_close(struct user_regs *regs);
long _x64_sys_unlink(struct user_regs *regs);
long _x64_sys_exit(struct user_regs *regs);
long _x64_sys_waitpid(struct user_regs *regs);
long _x64_sys_fork(struct user_regs *regs);
long _x64_sys_getpid(struct user_regs *regs);

#endif
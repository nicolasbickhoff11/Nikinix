#ifndef _USER_SCHED_FORK_FORK
#define _USER_SCHED_FORK_FORK 1

#include <stdint.h>

#include "user/sched/sched.h"
#include "syscall/regs.h"

uint64_t *virt_read_cr3(void);

int do_fork(
    task_t *parent,
    const struct user_regs *regs,
    task_t **child_out
);

#endif
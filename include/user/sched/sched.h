#ifndef _USER_SCHED_H
#define _USER_SCHED_H 1

#include <stdint.h>
#include "isr.h"
#include "vfs/vfs.h"

#define TIMER_TICK_MS 10
#define QUANTUM_TICKS 3

#define STATE_READY 0
#define STATE_RUNNING 1
#define STATE_STOPPED 2
#define STATE_WAITING 3
#define STATE_DEAD 4

#define MAX_FDS 256

#define FILE_NORMAL 0
#define FILE_DIR 1
#define FILE_CONSOLE 2

typedef struct file {
    vnode_t *vnode;
    uint64_t offset;
    int flags;
    int type;
    int refcount;
} file_t;

typedef struct task_struct {
    uint64_t pid;
    struct interrupt_frame frame;
    uint64_t kernel_rsp;
    uint64_t user_cr3;
    uint64_t quantum;
    uint64_t ticks;
    int state;
    file_t *fds[MAX_FDS];
    struct task_struct *next;
    struct task_struct *parent;
} task_t;

extern task_t *current;
extern task_t *start;

void scheduler(struct interrupt_frame *frame);

#endif
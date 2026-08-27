#ifndef _USER_SCHED_H
#define _USER_SCHED_H 1

#include <stdint.h>
#include "isr.h"

#define TIMER_TICK_MS 10
#define QUANTUM_TICKS 3

#define STATE_READY 0
#define STATE_RUNNING 1
#define STATE_STOPPED 2
#define STATE_WAITING 3
#define STATE_DEAD 4

typedef struct task_struct {
    uint64_t pid;
    struct interrupt_frame frame;
    uint64_t kernel_rsp;
    uint64_t *user_cr3;
    uint64_t quantum;
    uint64_t ticks;
    int state;
    struct task_struct *next;
} task_t;

extern task_t *current;
extern task_t *start;

void scheduler(struct interrupt_frame *frame);

#endif
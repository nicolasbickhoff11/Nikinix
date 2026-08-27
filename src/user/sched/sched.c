#include "user/sched/sched.h"
#include "isr.h"
#include <stdint.h>
#include <stddef.h>

task_t *current = NULL;
task_t *start = NULL;

void scheduler(struct interrupt_frame *frame) {
    task_t *p = NULL;
    if (!current->next)
        p = start;
    else
        p = current->next;

    for (;;) {
        if (p->state == STATE_READY) {
            current = p;
            current->ticks = 0;
            current->quantum = QUANTUM_TICKS;
            if (current->user_cr3) asm volatile ("mov %0, %%cr3" :: "r"(current->user_cr3) : "memory");
            *frame = current->frame;
            break;
        }

        if (!p->next) {
            if (start == p)
                return;

            p = start;
        } else {
            p = p->next;
        }
    }

}
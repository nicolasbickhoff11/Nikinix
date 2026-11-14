#include "user/sched/sched.h"
#include "isr.h"

void yield_handler(struct interrupt_frame *frame) {
    scheduler(frame);
}
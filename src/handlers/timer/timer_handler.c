#include "acpi/apic.h"
#include "user/sched/sched.h"
#include "print.h"
#include <stdint.h>

uint64_t timer_ticks = 0;
int preemptive_enabled = 0;

void preemptive_enable() {
    preemptive_enabled = 1;
}

void preemptive_disable() {
    preemptive_enabled = 0;
}

void timer_handler(struct interrupt_frame *frame) {
    timer_ticks++;

    int need_schedule = 0;

    if (current && preemptive_enabled) {
        current->ticks++;

        if (current->ticks >= current->quantum ||
            current->state != STATE_RUNNING) {
            need_schedule = 1;
        }
    }

    lapic_write(0xB0, 0);

    if (need_schedule)
        scheduler(frame);
}
#include "acpi/apic.h"
#include "user/sched/sched.h"
#include "print.h"
#include <stdint.h>

uint64_t timer_ticks = 0;

void timer_handler(struct interrupt_frame *frame) {
    timer_ticks++;
    //kprintf("tick %lld\n", timer_ticks);
    if (current && ((frame->cs & 0x3) == 3)) {
        current->ticks++;
        if (current->ticks >= current->quantum)
            scheduler(frame);
    }
    lapic_write(0xB0, 0);
}

#include "acpi/apic.h"
#include "print.h"
#include <stdint.h>

uint64_t timer_ticks = 0;

void timer_handler() {
    timer_ticks++;
    //kprintf("tick %lld\n", timer_ticks);
    lapic_write(0xB0, 0);
}

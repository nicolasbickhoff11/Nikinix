#ifndef _ACPI_APIC_H
#define _ACPI_APIC_H 1

#include <stdint.h>

#define LAPIC_LVT_TIMER     0x320
#define LAPIC_INITIAL_COUNT 0x380
#define LAPIC_CURRENT_COUNT 0x390
#define LAPIC_DIVIDE        0x3E0

#define LAPIC_LVT_MASKED    (1 << 16)
#define LAPIC_TIMER_PERIODIC (1 << 17)

extern uint64_t io_apic_address;
extern uint64_t lapic_address;
extern uint32_t *io_apic_virt;
extern uint32_t *lapic_virt;
extern int gsi_keyboard_ps2;
extern int irq1_polarity;
extern int irq1_trigger;

uint32_t ioapic_read(uint8_t reg);
void ioapic_write(uint8_t reg, uint32_t value);
uint64_t ioapic_read_redir(uint32_t gsi);
void ioapic_write_redir(uint32_t gsi, uint64_t value);
void read_madt_entries();
void lapic_write(uint32_t offset, uint32_t value);
void calibrate_lapic_timer(uint64_t miliseconds);
uint32_t lapic_read(uint32_t offset);

#endif
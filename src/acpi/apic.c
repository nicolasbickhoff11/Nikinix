#include "acpi/madt.h"
#include "acpi/apic.h"
#include "tsc.h"
#include <stdint.h>
#include "print.h"
#include "map.h"

uint64_t io_apic_address = 0;
uint64_t lapic_address = 0;
uint32_t *io_apic_virt = 0;
uint32_t *lapic_virt = 0;

int gsi_keyboard_ps2 = 1;
int irq1_polarity = 0;
int irq1_trigger = 0;

uint32_t ioapic_read(uint8_t reg)
{
    io_apic_virt[0] = reg;     // IOREGSEL (0x00)
    return io_apic_virt[4];    // IOWIN (0x10)
}

void ioapic_write(uint8_t reg, uint32_t value)
{
    io_apic_virt[0] = reg;     // IOREGSEL
    io_apic_virt[4] = value;   // IOWIN
}

uint64_t ioapic_read_redir(uint32_t gsi)
{
    uint32_t low;
    uint32_t high;

    low = ioapic_read(0x10 + gsi * 2);
    high = ioapic_read(0x11 + gsi * 2);

    return ((uint64_t)high << 32) | low;
}

void ioapic_write_redir(uint32_t gsi, uint64_t value)
{
    ioapic_write(0x10 + gsi * 2, (uint32_t)value);
    ioapic_write(0x11 + gsi * 2, (uint32_t)(value >> 32));
}

void lapic_write(uint32_t offset, uint32_t value) {
    lapic_virt[offset / 4] = value;
}

uint32_t lapic_read(uint32_t offset) {
    return lapic_virt[offset / 4];
}

void calibrate_lapic_timer(uint64_t miliseconds) {
    kprintf("[LOG] Setting LAPIC Timer to %lld ms ticks...\n", miliseconds);
    lapic_write(LAPIC_LVT_TIMER, 0x20 | LAPIC_TIMER_PERIODIC);
    lapic_write(LAPIC_DIVIDE, 0x3);
    asm volatile ("cli");
    lapic_write(LAPIC_INITIAL_COUNT, 0xFFFFFFFF);

    uint64_t start = rdtsc();
    uint64_t delay = (tsc_frequency * miliseconds) / 1000;
    while ((rdtsc() - start) < delay) {
        asm volatile ("pause");
    }

    uint32_t current = lapic_read(LAPIC_CURRENT_COUNT);

    uint32_t lapic_frequency = 0xFFFFFFFF - current;

    kprintf("[LOG] LAPIC Frequency in %lld ms: %d\n", miliseconds, lapic_frequency);

    lapic_write(LAPIC_INITIAL_COUNT, 0);
    lapic_write(LAPIC_LVT_TIMER, LAPIC_LVT_MASKED);
    lapic_write(LAPIC_LVT_TIMER, 0x20 | LAPIC_TIMER_PERIODIC);
    lapic_write(LAPIC_INITIAL_COUNT, lapic_frequency);
    asm volatile ("sti");

    kprintf("[LOG] Calibration and all done for LAPIC Timer\n");
}

void read_madt_entries() {
    struct MADT *madt = get_madt();
    uint8_t *p = madt->entries_data;
    uint8_t *end = (uint8_t *)madt + madt->h.Length;

    while (p < end) {
        struct madt_entry_header *h = (void *)p;

        switch (h->type) {
            case MADT_IO_APIC:
                struct madt_entry_io_apic *ioapic = (void *)h;
                io_apic_address = (uint64_t)ioapic->io_apic_addr;
                kprintf("IO APIC PHYS ADDR: 0x%llx\n", io_apic_address);
                kprintf("IO APIC ID: %u\n", ioapic->io_apic_id);
                kprintf("IO APIC GSI Base: %d\n", ioapic->gsi_base);
                gsi_keyboard_ps2 = ioapic->gsi_base + 1;
                break;

            case MADT_IO_APIC_INTERRUPT_SOURCE_OVERRIDE:
                struct madt_entry_io_apic_iso *iso = (void *)h;
                kprintf("ISO BUS SRC: %u\n", iso->bus_src);
                kprintf("ISO IRQ SRC: %u\n", iso->irq_src);
                kprintf("ISO GSI: %u\n", iso->gsi);
                if (iso->irq_src == 1) {
                    gsi_keyboard_ps2 = iso->gsi;
                    uint16_t flags = iso->flags;

                    irq1_polarity = flags & 0x3;
                    irq1_trigger  = (flags >> 2) & 0x3;
                }
                break;
        }

        p += h->record_length;
    }
}
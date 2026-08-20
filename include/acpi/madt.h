#ifndef _ACPI_MADT_H
#define _ACPI_MADT_H 1

#include "acpi/sdt.h"
#include <stdint.h>

extern uint64_t madt_addr;

#define MADT_LOCAL_APIC                        0
#define MADT_IO_APIC                           1
#define MADT_IO_APIC_INTERRUPT_SOURCE_OVERRIDE 2
#define MADT_IO_APIC_NMI_SOURCE                3
#define MADT_LOCAL_APIC_NMI                    4
#define MADT_LOCAL_APIC_ADDRESS_OVERRIDE       5
#define MADT_LOCAL_X2APIC                      9

struct madt_entry_header {
    uint8_t type;
    uint8_t record_length;
} __attribute__((packed));

struct madt_entry_local_apic {
    struct madt_entry_header h;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

struct madt_entry_io_apic {
    struct madt_entry_header h;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_addr;
    uint32_t gsi_base;
} __attribute__((packed));

struct madt_entry_io_apic_iso {
    struct madt_entry_header h;
    uint8_t bus_src;
    uint8_t irq_src;
    uint32_t gsi;
    uint16_t flags;
} __attribute__((packed));

struct madt_entry_io_apic_nmi_src {
    struct madt_entry_header h;
    uint8_t nmi_src;
    uint8_t reserved;
    uint16_t flags;
    uint32_t gsi;
} __attribute__((packed));

struct madt_entry_local_apic_nmi {
    struct madt_entry_header h;
    uint8_t acpi_processor_id;
    uint16_t flags;
    uint8_t lint;
} __attribute__((packed));

struct madt_entry_local_apic_addr_override {
    struct madt_entry_header h;
    uint16_t reserved;
    uint64_t lapic_address;
} __attribute__((packed));

struct madt_entry_local_x2apic {
    struct madt_entry_header h;
    uint16_t reserved;
    uint32_t local_x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__((packed));

struct MADT {
    struct SDT_header h;
    uint32_t local_apic_addr;
    uint32_t flags;
    uint8_t entries_data[];
} __attribute__((packed));

struct MADT *get_madt();
int check_madt();

#endif
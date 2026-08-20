#ifndef _ACPI_PCIE_H
#define _ACPI_PCIE_H

#include <stdint.h>

extern uint64_t pcie_base;

struct pci_config_header
{
    uint16_t vendor_id;
    uint16_t device_id;

    uint16_t command;
    uint16_t status;

    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;

    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
};

struct pci_header_type0
{
    struct pci_config_header h;

    // 0x10
    uint32_t bar[6];

    // 0x28
    uint32_t cardbus_cis_pointer;

    // 0x2C
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;

    // 0x30
    uint32_t expansion_rom_base;

    // 0x34
    uint8_t capabilities_pointer;
    uint8_t reserved1[3];

    // 0x38
    uint32_t reserved2;

    // 0x3C
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
} __attribute__((packed));

struct pci_header_type1
{
    struct pci_config_header h;

    uint32_t bar[2];

    uint8_t primary_bus;
    uint8_t secondary_bus;
    uint8_t subordinate_bus;
    uint8_t secondary_latency_timer;

    uint8_t io_base;
    uint8_t io_limit;
    uint16_t secondary_status;

    uint16_t memory_base;
    uint16_t memory_limit;

    uint16_t prefetchable_memory_base;
    uint16_t prefetchable_memory_limit;

    uint32_t prefetchable_base_upper32;
    uint32_t prefetchable_limit_upper32;

    uint16_t io_base_upper16;
    uint16_t io_limit_upper16;

    uint8_t capabilities_pointer;
    uint8_t reserved[3];

    uint32_t expansion_rom_base;

    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint16_t bridge_control;
} __attribute__((packed));

uint64_t pci_ecam_addr(
    uint64_t ecam,
    uint8_t bus,
    uint8_t device,
    uint8_t function
);
struct pci_config_header *pci_read(uint64_t ecam, uint8_t bus, uint8_t device, uint8_t function);
uint64_t pci_read_bar64(struct pci_header_type0 *header, uint8_t bar);
void pci_scan_bus(uint64_t ecam, uint8_t bus);
uint64_t get_pcie();
void read_pcie_addr();

#endif

#ifndef _ACPI_MCFG_H
#define _ACPI_MCFG_H 1

#include "acpi/sdt.h"
#include <stdint.h>

extern uint64_t mcfg_addr;

struct base_alloc_struct {
    uint64_t base_addr;
    uint16_t pci_segment_group_num;
    uint8_t start_pci_bus_num;
    uint8_t end_pci_bus_num;
    uint32_t reserved;
} __attribute__((packed));

struct MCFG {
    struct SDT_header h;
    uint64_t reserved;
    struct base_alloc_struct allocations[];
} __attribute__((packed));

struct MCFG *get_mcfg();
int check_mcfg();

#endif
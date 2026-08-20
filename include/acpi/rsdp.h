#ifndef _ACPI_RSDP_H
#define _ACPI_RSDP_H 1

#include <stdint.h>

struct rsdp_xsdp {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;      // deprecated since version 2.0
// ACPI 2.0+
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

extern uint32_t acpi_version;

int check_rsdp();
struct rsdp_xsdp *get_rsdp();

#endif

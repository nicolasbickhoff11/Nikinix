#ifndef _ACPI_CHECKSUM_H
#define _ACPI_CHECKSUM_H 1

#include <stdint.h>

uint8_t check_checksum(uint8_t *bytes, uint32_t length);

#endif

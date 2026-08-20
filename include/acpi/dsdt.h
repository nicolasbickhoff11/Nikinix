#ifndef _ACPI_DSDT_H
#define _ACPI_DSDT_H 1

#include <stdint.h>

#include "acpi/sdt.h"

extern uint64_t dsdt_addr;
struct SDT_header *get_dsdt();
int check_dsdt();

#endif
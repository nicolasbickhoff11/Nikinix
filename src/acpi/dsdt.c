#include "acpi/rsdp.h"
#include "acpi/sdt.h"
#include "acpi/dsdt.h"
#include "acpi/checksum.h"
#include "acpi/fadt.h"

#include "print.h"
#include "map.h"
#include <stdint.h>
#include "string.h"

uint64_t dsdt_addr = 0;

struct SDT_header *get_dsdt() {
    return (struct SDT_header *)dsdt_addr;
}

int check_dsdt() {
    struct FADT *fadt = get_fadt();

    if (acpi_version < 2) {
        dsdt_addr = (uint64_t)phys_to_virt((uint64_t)fadt->Dsdt);
    } else {
        dsdt_addr = (uint64_t)phys_to_virt(fadt->X_Dsdt);
    }

    struct SDT_header *dsdt = get_dsdt();
    return (check_checksum((uint8_t *)dsdt, dsdt->Length) == 0) ? 1 : 0;
}

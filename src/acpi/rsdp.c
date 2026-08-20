#include "rsdp_addr.h"
#include "acpi/rsdp.h"
#include "acpi/checksum.h"
#include "map.h"
#include "print.h"

#include <stdint.h>

uint32_t acpi_version = 0;

struct rsdp_xsdp *get_rsdp() {
    return (struct rsdp_xsdp *)phys_to_virt(rsdp_phys_address);
}

int check_rsdp() {
    struct rsdp_xsdp *rsdp = get_rsdp();
    int valid = 0;

    if (rsdp->Revision >= 2) {
        if (check_checksum((uint8_t*)rsdp, rsdp->Length) == 0) {
            valid = 1;
        }
    } else if (rsdp->Revision < 2) {
        if (check_checksum((uint8_t*)rsdp, 20) == 0) {
            valid = 1;
        }
    }

    acpi_version = rsdp->Revision;

    if (acpi_version < 2) {
        kprintf("[ACPI] Detected ACPI 1.0!\n");
    } else {
        kprintf("[ACPI] Detected ACPI 2.0+!\n");
    }
    return valid;
}




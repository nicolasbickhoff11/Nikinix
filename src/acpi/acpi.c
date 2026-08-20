#include "acpi/rsdp.h"
#include "acpi/xsdt_rsdt.h"
#include "acpi/fadt.h"
#include "acpi/dsdt.h"
#include "acpi/madt.h"

#include "print.h"

#include <stdint.h>

int init_acpi() {
    if (check_rsdp() == 1) {
        kprintf("[ACPI] RSDP is valid!\n");
    } else {
        kprintf("[ACPI] RSDP is invalid!\n");
        return 1;
    }

    if (check_rsdt_xsdt() == 1) {
        kprintf("[ACPI] RSDT/XSDT is valid!\n");
    } else {
        kprintf("[ACPI] RSDT/XSDT in invalid!\n");
        return 1;
    }

    if (check_fadt() == 1) {
        kprintf("[ACPI] FADT is valid!\n");
    } else {
        kprintf("[ACPI] FADT is invalid\n");
        return 1;
    }

    if (check_dsdt() == 1) {
        kprintf("[ACPI] DSDT is valid!\n");
    } else {
        kprintf("[ACPI] DSDT is invalid!\n");
        return 1;
    }

    if (check_madt() == 1) {
        kprintf("[ACPI] MADT is valid!\n");
    } else {
        kprintf("[ACPI] MADT is invalid!\n");
        return 0;
    }

    read_madt_entries();
    return 0;
}
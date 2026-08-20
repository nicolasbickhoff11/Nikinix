#include <stdint.h>
#include "io.h"
#include "map.h"
#include "acpi/fadt.h"
#include "acpi/shutdown.h"

void acpi_shutdown() {
    struct FADT *fadt = get_fadt();
    uint64_t poweroff_address = fadt->X_PM1aControlBlock.Address;
    uint16_t value = (slp_typa << 10) | (1 << 13);
    if (fadt->X_PM1aControlBlock.AddressSpace == 0) {
        *(uint16_t*)phys_to_virt(poweroff_address) = value;
    } else if (fadt->X_PM1aControlBlock.AddressSpace == 1) {
        outw((uint16_t)poweroff_address, value);
    }
}
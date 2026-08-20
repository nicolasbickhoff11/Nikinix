#include "acpi/fadt.h"
#include "acpi/sdt.h"

#include "io.h"
#include "map.h"

#include <stdint.h>

void acpi_reboot() {
    struct FADT *fadt = get_fadt();
    uint8_t reset_val = fadt->ResetValue;
    uint64_t reset_addr = fadt->ResetReg.Address;
    if (fadt->ResetReg.AddressSpace == 0) {
        *(uint8_t*)phys_to_virt(reset_addr) = reset_val;
    } else if (fadt->ResetReg.AddressSpace == 1) {
        outb((uint16_t)reset_addr, reset_val);
    }
}
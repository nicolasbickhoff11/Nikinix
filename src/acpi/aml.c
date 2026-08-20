#include "acpi/dsdt.h"
#include "acpi/sdt.h"

#include <stdint.h>
#include "string.h"
#include "io.h"
#include "map.h"

uint8_t slp_typa = 0;
uint8_t slp_typb = 0;

int find_shutdown_values() {
    struct SDT_header *dsdt = get_dsdt();
    uint8_t *aml = (uint8_t*)((uint64_t)dsdt + sizeof(struct SDT_header));
    uint64_t byte_count = dsdt->Length - sizeof(struct SDT_header);
    for (uint64_t i = 0; i < byte_count - 5; i++) {
        if (aml[i] != 0x08)
            continue;

        uint64_t pos = i + 1;

        if (aml[pos] == 0x5C)
            pos++;

        if (memcmp(&aml[pos], "_S5_", 4) != 0)
            continue;

        pos += 4;

        if (aml[pos] != 0x12)
            continue;

        pos++; // PackageOp

        pos++; // PkgLength
        pos++; // NumElements

        // Primeiro elemento
        if (aml[pos] == 0x0A) {
            pos++;
            slp_typa = aml[pos];
        }

        pos++;

        // Segundo elemento
        if (aml[pos] == 0x0A)
        {
            pos++;
            slp_typb = aml[pos];
        }

    }
    return 0;
}
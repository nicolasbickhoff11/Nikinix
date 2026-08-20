#include "acpi/rsdp.h"
#include "acpi/checksum.h"
#include "acpi/mcfg.h"
#include "acpi/xsdt_rsdt.h"

#include "map.h"
#include "string.h"

uint64_t mcfg_addr = 0;

struct MCFG *get_mcfg() {
    return (struct MCFG *)mcfg_addr;
}

int check_mcfg() {
    if (acpi_version < 2) {
        struct SDT_header *rsdt = get_rsdt();
        uint32_t *entries = (uint32_t *)((uint8_t*)rsdt + sizeof(struct SDT_header));
        int entries_count = (rsdt->Length - sizeof(struct SDT_header)) / 4;
        for (int i = 0; i < entries_count; i++) {
            struct SDT_header *entry = (struct SDT_header *)phys_to_virt((uint64_t)entries[i]);
            if (memcmp(entry->Signature, "MCFG", 4) == 0) {
                mcfg_addr = (uint64_t)entry;
            }
        }
    } else {
        struct SDT_header *xsdt = get_xsdt();
        uint64_t *entries = (uint64_t *)((uint8_t*)xsdt + sizeof(struct SDT_header));
        int entries_count = (xsdt->Length - sizeof(struct SDT_header)) / 8;
        for (int i = 0; i < entries_count; i++) {
            struct SDT_header *entry = (struct SDT_header *)phys_to_virt((uint64_t)entries[i]);
            if (memcmp(entry->Signature, "MCFG", 4) == 0) {
                mcfg_addr = (uint64_t)entry;
            }
        }
    }

    struct MCFG *mcfg = get_mcfg();
    return (check_checksum((uint8_t *)mcfg, mcfg->h.Length) == 0) ? 1 : 0;
}

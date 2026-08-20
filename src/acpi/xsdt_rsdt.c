#include "acpi/rsdp.h"
#include "acpi/sdt.h"
#include "acpi/checksum.h"
#include <stdint.h>
#include "map.h"

struct SDT_header *get_rsdt() {
    struct rsdp_xsdp *rsdp = get_rsdp();
    return (struct SDT_header*)phys_to_virt((uint64_t)rsdp->RsdtAddress);
}

struct SDT_header *get_xsdt() {
    struct rsdp_xsdp *rsdp = get_rsdp();
    return (struct SDT_header*)phys_to_virt(rsdp->XsdtAddress);
}

int check_rsdt_xsdt() {
    if (acpi_version < 2) {
        struct SDT_header *rsdt = get_rsdt();
        return (check_checksum((uint8_t*)rsdt, rsdt->Length) == 0) ? 1 : 0;
    } else {
        struct SDT_header *xsdt = get_xsdt();
        return (check_checksum((uint8_t*)xsdt, xsdt->Length) == 0) ? 1 : 0;
    }
    return 0;
}
#ifndef _ACPI_XSDT_RSDT
#define _ACPI_XSDT_RSDT 1

struct SDT_header *get_rsdt();
struct SDT_header *get_xsdt();
int check_rsdt_xsdt();

#endif
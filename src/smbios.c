#include "smbios.h"
#include <stdint.h>
#include <stddef.h>

#include "map.h"
#include "print.h"

uint64_t entry_64 = 0;
uint64_t entry_32 = 0;

struct SMBIOSInfo info = {0};

char *smbios_get_string(char *strings, uint8_t index)
{
    if (index == 0)
        return NULL;

    char *s = strings;

    for (uint8_t i = 1; i < index; i++) {
        while (*s)
            s++;

        if (*(uint8_t *)s == 0)
            s++; // skip null terminator
    }

    return s;
}

uint8_t *get_next(struct SMBIOSHeader *header) {
    uint8_t *p = (uint8_t *)header + header->length;

    while (!(p[0] == 0 && p[1] == 0)) {
        p++;
    }

    return p + 2;
}


void type0_smbios(void *p) {
    struct SMBIOSType0 *type0 = (struct SMBIOSType0 *)p;
    char *strings = (char*)((uintptr_t)p + type0->h.length);
    info.bios_vendor = smbios_get_string(strings, type0->vendor);
    info.bios_version = smbios_get_string(strings, type0->bios_version);
    info.bios_release_date = smbios_get_string(strings, type0->bios_release_date);
}

void type1_smbios(void *p) {
    struct SMBIOSType1 *type1 = (struct SMBIOSType1 *)p;
    char *strings = (char*)((uintptr_t)p + type1->h.length);
    info.manufacturer = smbios_get_string(strings, type1->Manufacturer);
    info.productname = smbios_get_string(strings, type1->ProductName);
    info.version = smbios_get_string(strings, type1->Version);
    info.serialnumber = smbios_get_string(strings, type1->SerialNumber);
}

void smbios_walk(uint8_t *start, uint8_t *end)
{
    uint8_t *p = start;

    while (p < end)
    {
        struct SMBIOSHeader *h = (struct SMBIOSHeader *)p;

        if (h->type == 127) // End-of-table
            break;

        switch (h->type)
        {
            case 0:
                type0_smbios(p);
                break;

            case 1:
                type1_smbios(p);
                break;

            case 2:
                // Baseboard
                break;

            case 4:
                // CPU info
                break;

            case 17:
                // Memory Device (RAM slots)
                break;
        }

        p = get_next(h);
    }
}

void smbios_init()
{
    uint8_t *base;
    uint64_t table_addr;
    uint32_t table_size;

    // -------- SMBIOS 3.x --------
    struct SMBIOS_64 *e64 = (struct SMBIOS_64 *)entry_64;

    if (e64 &&
        e64->anchor_str[0] == '_' &&
        e64->anchor_str[1] == 'S' &&
        e64->anchor_str[2] == 'M')
    {
        kprintf("[SMBIOS] detected SMBIOS 3.x %u.%u\n",
                e64->major, e64->minor);

        table_addr = e64->table_addr;
        table_size = e64->table_max_size;
    }
    else
    {
        // -------- FALLBACK SMBIOS 2.x --------
        struct SMBIOS_32 *e32 = (struct SMBIOS_32 *)entry_32;

        if (!e32 ||
            e32->anchor[0] != '_' ||
            e32->anchor[1] != 'S')
        {
            kprintf("[SMBIOS] not found (3.x or 2.x)\n");
            return;
        }

        kprintf("[SMBIOS] fallback SMBIOS 2.x %u.%u\n",
                e32->major, e32->minor);

        table_addr = e32->table_address;
        table_size = e32->table_length;
    }

    base = (uint8_t *)phys_to_virt(table_addr);

    smbios_walk(base, base + table_size);
}
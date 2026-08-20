#ifndef _SMBIOS_H
#define _SMBIOS_H 1

#include <stdint.h>

extern uint64_t entry_64;
extern uint64_t entry_32;

struct SMBIOS_64 {
    uint8_t anchor_str[5];
    uint8_t checksum;
    uint8_t entry_length;
    uint8_t major;
    uint8_t minor;
    uint8_t docrev;
    uint8_t entry_revision;
    uint8_t reserved;
    uint32_t table_max_size;
    uint64_t table_addr;
} __attribute__((packed));

struct SMBIOSHeader {
    uint8_t type;
    uint8_t length;
    uint16_t handle;
} __attribute__((packed));

struct SMBIOSType0 {
    struct SMBIOSHeader h;

    uint8_t  vendor;        // string index (na área de strings)
    uint8_t  bios_version;  // string index
    uint16_t bios_starting_address_segment;
    uint8_t  bios_release_date; // string index
    uint8_t  bios_rom_size;     // em 64KB blocks

    uint64_t  characteristics;

    uint8_t  extension_bytes[2]; // SMBIOS 2.3+
    uint8_t  major_release;
    uint8_t  minor_release;
    uint8_t  embedded_controller_major;
    uint8_t  embedded_controller_minor;

} __attribute__((packed));

struct SMBIOSType1 {
    struct SMBIOSHeader h;

    uint8_t Manufacturer;
    uint8_t ProductName;
    uint8_t Version;
    uint8_t SerialNumber;
} __attribute__((packed));

struct SMBIOSInfo {
    char *manufacturer;
    char *productname;
    char *version;
    char *serialnumber;
    char *bios_vendor;
    char *bios_version;
    char *bios_release_date;
};

struct SMBIOS_32 {
    char anchor[4];                 // "_SM_"
    uint8_t checksum;
    uint8_t length;
    uint8_t major;
    uint8_t minor;
    uint16_t max_structure_size;
    uint8_t entry_point_revision;
    uint8_t formatted_area[5];

    char intermediate_anchor[5];    // "_DMI_"
    uint8_t intermediate_checksum;

    uint16_t table_length;
    uint32_t table_address;

    uint16_t structure_count;
    uint8_t bcd_revision;
} __attribute__((packed));

extern struct SMBIOSInfo info;

void smbios_init();

#endif
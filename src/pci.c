#include <stdint.h>
#include "io.h"
#include "pci.h"

uint32_t pci_config_read_DWORD(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t Addr;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint32_t temp = 0;

    Addr =
    (uin32_t)
    (
        (lbus << 16) | (lslot << 11) | (lfunc << 8) |
        (offset & 0xfc) | ((DWORD)0x80000000)
    );

    outl(PCI_CONFIG_ADDRESS, Addr);
    temp = inl(PCI_CONFIG_DATA);

    return temp;
}

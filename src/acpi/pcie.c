#include "acpi/mcfg.h"
#include "acpi/pcie.h"
#include "print.h"
#include "map.h"
#include <stdint.h>


uint64_t pcie_base = 0;


uint64_t pci_ecam_addr(
    uint64_t ecam,
    uint8_t bus,
    uint8_t device,
    uint8_t function
)
{
    return ecam
        + ((uint64_t)bus << 20)
        + ((uint64_t)device << 15)
        + ((uint64_t)function << 12);
}

struct pci_config_header *pci_read(uint64_t ecam, uint8_t bus, uint8_t device, uint8_t function) {
    return (void *)pci_ecam_addr(ecam, bus, device, function);
}

uint64_t pci_read_bar64(struct pci_header_type0 *header, uint8_t bar) {
    uint32_t low = header->bar[bar];
    if (low & 1)
        return 0;

    if (((low >> 1) & 3) == 2) {
        uint32_t high = header->bar[bar + 1];

        return ((uint64_t)high << 32) | (low & ~0xF);
    }

    return low & ~0xF;
}


void pci_scan_bus(
    uint64_t ecam,
    uint8_t bus
)
{
    for (uint8_t dev = 0; dev < 32; dev++)
    {
        for (uint8_t fn = 0; fn < 8; fn++)
        {
            uint64_t addr =
                pci_ecam_addr(
                    ecam,
                    bus,
                    dev,
                    fn
                );


            struct pci_config_header *pci =
                (struct pci_config_header *)addr;


            if (pci->vendor_id == 0xFFFF)
                continue;


            kprintf(
                "PCIe %u:%u:%u "
                "Vendor=%x Device=%x "
                "Class=%x Sub=%x Prog IF=%x\n",
                bus,
                dev,
                fn,
                pci->vendor_id,
                pci->device_id,
                pci->class_code,
                pci->subclass,
                pci->prog_if
            );


            /*
             * PCI Bridge / PCIe Root Port
             * Class 06 Subclass 04
             */
            if (pci->class_code == 0x06 &&
                pci->subclass == 0x04)
            {
                struct pci_header_type1 *p = (void *)pci;
                uint32_t buses =
                    p->bar[1];


                uint8_t primary =
                    buses & 0xff;

                uint8_t secondary =
                    (buses >> 8) & 0xff;

                uint8_t subordinate =
                    (buses >> 16) & 0xff;


                kprintf(
                    "  Bridge: "
                    "Primary=%x "
                    "Secondary=%x "
                    "Subordinate=%x\n",
                    primary,
                    secondary,
                    subordinate
                );


                /*
                 * Escanear o barramento
                 * atrás do Root Port
                 */
                if (secondary != 0 &&
                    secondary != bus)
                {
                    pci_scan_bus(
                        ecam,
                        secondary
                    );
                }
            }


            /*
             * Exemplo USB
             */
            if (pci->class_code == 0x0C &&
                pci->subclass == 0x03)
            {
                kprintf(
                    "USB controller found!\n"
                );

                struct pci_header_type0 *p = (void *)pci;
                uint64_t complete_bar = pci_read_bar64(p, 0);
                kprintf("USB PHYS BAR: 0x%llx\n", complete_bar);
            }

            if (pci->class_code == 0x03 &&
                pci->subclass == 0x00)
            {
                kprintf(
                    "VGA-Compatible GPU Found!\n"
                );
                struct pci_header_type0 *p = (void *)pci;
                uint64_t bar0 = pci_read_bar64(p, 0);
                uint64_t bar1 = pci_read_bar64(p, 1);
                uint64_t bar2 = pci_read_bar64(p, 2);
                kprintf("VGA PHYS BARs: 0x%llx, 0x%llx, 0x%llx\n", bar0, bar1, bar2);
                if (bar1 == virt_to_phys(framebuffer)) {
                    kprintf("BAR1 Matches PHYS Framebuffer!\n");
                } else if (bar2 == virt_to_phys(framebuffer)) {
                    kprintf("BAR2 Matches PHYS Framebuffer!\n");
                }

            }
        }
    }
}


uint64_t get_pcie()
{
    return pcie_base;
}


void read_pcie_addr()
{
    if (check_mcfg() == 1)
    {
        struct MCFG *mcfg =
            get_mcfg();


        pcie_base =
            phys_to_virt(
                mcfg->allocations[0].base_addr
            );


        map_range_generic(
            pml4,
            pcie_base,
            mcfg->allocations[0].base_addr,
            PAGE_PRESENT | PAGE_RW,
            0x1400000
        );


        kprintf(
            "PCIe ECAM base: %lx\n",
            pcie_base
        );


        /*
         * Começa pelo Bus 0
         */
//        pci_scan_bus(
//            pcie_base,
//            0
//        );
    }
}

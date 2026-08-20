#include <stdint.h>
#include <stddef.h>
#include "map.h"
#include "pmm.h"
#include "string.h"

uint64_t *pml4 = 0;

uint64_t mem_from_alloc = 0;
uint64_t mem_end        = 0;

void *phys_to_virt(uint64_t phys) {
    return (void*)(phys + hhdm_offset);
}

uint64_t virt_to_phys(void *virt) {
    return (uint64_t)(virt - hhdm_offset);
}

int map_generic(uint64_t *generic_pml4, uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t pml4i = (virt >> 39) & 0x1FF;
    uint64_t pdpti = (virt >> 30) & 0x1FF;
    uint64_t pdi   = (virt >> 21) & 0x1FF;
    uint64_t pti   = (virt >> 12) & 0x1FF;


    /*
     * PML4 -> PDPT
     */

    if (!(generic_pml4[pml4i] & PAGE_PRESENT)) {

        uint64_t pdpt_phys = bitmap_alloc(1);

        if (!pdpt_phys)
            return -1;


        uint64_t *pdpt =
            (uint64_t *)phys_to_virt(pdpt_phys);


        memset(pdpt, 0, 4096);


        generic_pml4[pml4i] =
            pdpt_phys | flags;
    }


    uint64_t pdpt_phys =
        generic_pml4[pml4i] & 0xFFFFFFFFFFFFF000;


    uint64_t *pdpt =
        (uint64_t *)phys_to_virt(pdpt_phys);



    /*
     * PDPT -> PD
     */

    if (!(pdpt[pdpti] & PAGE_PRESENT)) {

        uint64_t pd_phys = bitmap_alloc(1);

        if (!pd_phys)
            return -1;


        uint64_t *pd =
            (uint64_t *)phys_to_virt(pd_phys);


        memset(pd, 0, 4096);


        pdpt[pdpti] =
            pd_phys | flags;
    }


    uint64_t pd_phys =
        pdpt[pdpti] & 0xFFFFFFFFFFFFF000;


    uint64_t *pd =
        (uint64_t *)phys_to_virt(pd_phys);



    /*
     * PD -> PT
     */

    if (!(pd[pdi] & PAGE_PRESENT)) {

        uint64_t pt_phys = bitmap_alloc(1);

        if (!pt_phys)
            return -1;


        uint64_t *pt =
            (uint64_t *)phys_to_virt(pt_phys);


        memset(pt, 0, 4096);


        pd[pdi] =
            pt_phys | flags;
    }


    uint64_t pt_phys =
        pd[pdi] & 0xFFFFFFFFFFFFF000;


    uint64_t *pt =
        (uint64_t *)phys_to_virt(pt_phys);



    /*
     * PT -> Página final
     */

    if (!(pt[pti] & PAGE_PRESENT)) {

        pt[pti] =
            (phys & 0xFFFFFFFFFFFFF000) |
            flags |
            PAGE_PRESENT;
    }


    asm volatile(
        "invlpg (%0)"
        :
        :
        "r"(virt)
        :
        "memory"
    );


    return 0;
}

int map_range_generic(uint64_t *generic_pml4, uint64_t virt, uint64_t phys, uint64_t flags, size_t size) {
    size_t size_aligned = (size + 4095) & ~4095;
    for (size_t i = 0; i < size_aligned; i += 0x1000) {
        if (map_generic(generic_pml4, virt + (uint64_t)i, phys + (uint64_t)i, flags) == -1) return -1;
    }
    return 0;
}

int unmap_range_generic(uint64_t *generic_pml4, uint64_t virt, size_t size) {
    size_t size_aligned = (size + 4095) & ~4095;
    for (size_t i = 0; i < size_aligned; i += 0x1000) {
        if (unmap_generic(generic_pml4, virt + (uint64_t)i) == -1) return -1;
    }
    return 0;
}

int unmap_generic(uint64_t *generic_pml4, uint64_t virt) {
    uint64_t pml4i = (virt >> 39) & 0x1FF;
    uint64_t pdpti = (virt >> 30) & 0x1FF;
    uint64_t pdi = (virt >> 21) & 0x1FF;
    uint64_t pti = (virt >> 12) & 0x1FF;

    if (!(generic_pml4[pml4i] & 1)) {
        return -1;
    }

    uint64_t pdpt_phys = generic_pml4[pml4i] & 0xFFFFFFFFFFFFF000;
    uint64_t *pdpt = (uint64_t*)phys_to_virt(pdpt_phys);

    if (!(pdpt[pdpti] & 1)) {
        return -1;
    }

    uint64_t pd_phys = pdpt[pdpti] & 0xFFFFFFFFFFFFF000;
    uint64_t *pd = (uint64_t*)phys_to_virt(pd_phys);

    if (!(pd[pdi] & 1)) {
        return -1;
    }

    uint64_t pt_phys = pd[pdi] & 0xFFFFFFFFFFFFF000;
    uint64_t *pt = (uint64_t*)phys_to_virt(pt_phys);

    if (!(pt[pti] & 1)) {
        return -1;
    }

    pt[pti] = 0;

    asm volatile ("invlpg (%0)" :: "r"(virt) : "memory");

    return 0;
}

uint64_t get_phys_generic(uint64_t *generic_pml4, uint64_t virt) {
    uint64_t pml4i = (virt >> 39) & 0x1FF;
    uint64_t pdpti = (virt >> 30) & 0x1FF;
    uint64_t pdi = (virt >> 21) & 0x1FF;
    uint64_t pti = (virt >> 12) & 0x1FF;

    if (!(generic_pml4[pml4i] & 1)) {
        return -1;
    }

    uint64_t pdpt_phys = generic_pml4[pml4i] & 0xFFFFFFFFFFFFF000;
    uint64_t *pdpt = (uint64_t*)phys_to_virt(pdpt_phys);

    if (!(pdpt[pdpti] & 1)) {
        return -1;
    }

    uint64_t pd_phys = pdpt[pdpti] & 0xFFFFFFFFFFFFF000;
    uint64_t *pd = (uint64_t*)phys_to_virt(pd_phys);

    if (!(pd[pdi] & 1)) {
        return -1;
    }

    uint64_t pt_phys = pd[pdi] & 0xFFFFFFFFFFFFF000;
    uint64_t *pt = (uint64_t*)phys_to_virt(pt_phys);

    if (!(pt[pti] & 1)) {
        return -1;
    }

    return (pt[pti] & 0x000FFFFFFFFFF000ULL);
}

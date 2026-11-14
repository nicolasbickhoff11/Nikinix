
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

#include "map.h"
#include "pmm.h"
#include "string.h"

#ifndef PAGE_PRESENT
#define PAGE_PRESENT (1ULL << 0)
#define PAGE_RW      (1ULL << 1)
#define PAGE_USER    (1ULL << 2)
#define PAGE_PWT     (1ULL << 3)
#define PAGE_PCD     (1ULL << 4)
#define PAGE_NX      (1ULL << 63)
#endif

#define PAGE_SIZE 4096ULL
#define PAGE_ADDR 0x000FFFFFFFFFF000ULL
#define PAGE_PS   (1ULL << 7)

#define PML4_INDEX(v) (((v) >> 39) & 0x1FF)
#define PDPT_INDEX(v) (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)   (((v) >> 21) & 0x1FF)
#define PT_INDEX(v)   (((v) >> 12) & 0x1FF)

#define TABLE_FLAGS(f) \
    (PAGE_PRESENT | PAGE_RW | ((f) & PAGE_USER))

#define LEAF_FLAGS(f) \
    ((f) & (PAGE_RW | PAGE_USER | PAGE_PWT | PAGE_PCD | PAGE_NX))

uint64_t *pml4 = 0;

uint64_t mem_from_alloc = 0;
uint64_t mem_end = 0;


/* =========================================================
   HHDM
   ========================================================= */

void *phys_to_virt(uint64_t phys)
{
    return (void *)(uintptr_t)(phys + hhdm_offset);
}

uint64_t virt_to_phys(void *virt)
{
    return (uint64_t)(uintptr_t)virt - hhdm_offset;
}


/* =========================================================
   ADDRESS VALIDATION
   ========================================================= */

static bool is_canonical(uint64_t addr)
{
    uint64_t upper = addr >> 47;

    return upper == 0 || upper == 0x1FFFF;
}


/* =========================================================
   PAGE TABLE WALK: 4KB PAGES ONLY
   ========================================================= */

static int walk_4k(
    uint64_t *generic_pml4,
    uint64_t virt,
    uint64_t **leaf_out
)
{
    if (!generic_pml4 || !is_canonical(virt))
        return -1;

    uint64_t entry;

    entry = generic_pml4[PML4_INDEX(virt)];

    if (!(entry & PAGE_PRESENT))
        return -1;

    uint64_t *pdpt = phys_to_virt(entry & PAGE_ADDR);

    entry = pdpt[PDPT_INDEX(virt)];

    if (!(entry & PAGE_PRESENT) || (entry & PAGE_PS))
        return -1;

    uint64_t *pd = phys_to_virt(entry & PAGE_ADDR);

    entry = pd[PD_INDEX(virt)];

    if (!(entry & PAGE_PRESENT) || (entry & PAGE_PS))
        return -1;

    uint64_t *pt = phys_to_virt(entry & PAGE_ADDR);

    if (!(pt[PT_INDEX(virt)] & PAGE_PRESENT))
        return -1;

    if (leaf_out)
        *leaf_out = &pt[PT_INDEX(virt)];

    return 0;
}


/* =========================================================
   MAP GENERIC
   ========================================================= */

static void rollback_tables(
    uint64_t **entries,
    uint64_t *phys,
    int count
)
{
    while (count > 0) {
        count--;

        *entries[count] = 0;

        bitmap_free(phys[count], 1);
    }
}


int map_generic(
    uint64_t *generic_pml4,
    uint64_t virt,
    uint64_t phys,
    uint64_t flags
)
{
    if (!generic_pml4)
        return -1;

    if (!is_canonical(virt))
        return -1;

    if ((virt & 0xFFF) || (phys & 0xFFF))
        return -1;

    if (phys & ~PAGE_ADDR)
        return -1;

    uint64_t pml4i = PML4_INDEX(virt);
    uint64_t pdpti = PDPT_INDEX(virt);
    uint64_t pdi   = PD_INDEX(virt);
    uint64_t pti   = PT_INDEX(virt);

    uint64_t *created_entries[3];
    uint64_t created_phys[3];

    int created = 0;

    uint64_t *entry = &generic_pml4[pml4i];


    /* PML4 -> PDPT */

    if (!(*entry & PAGE_PRESENT)) {

        uint64_t new_phys = bitmap_alloc(1);

        if (!new_phys)
            goto fail;

        uint64_t *table = phys_to_virt(new_phys);

        memset(table, 0, PAGE_SIZE);

        *entry = new_phys | TABLE_FLAGS(flags);

        created_entries[created] = entry;
        created_phys[created++] = new_phys;

    } else {

        *entry |= PAGE_RW;

        if (flags & PAGE_USER)
            *entry |= PAGE_USER;
    }


    uint64_t *pdpt =
        phys_to_virt(*entry & PAGE_ADDR);


    /* PDPT -> PD */

    entry = &pdpt[pdpti];

    if (*entry & PAGE_PS)
        goto fail;

    if (!(*entry & PAGE_PRESENT)) {

        uint64_t new_phys = bitmap_alloc(1);

        if (!new_phys)
            goto fail;

        uint64_t *table = phys_to_virt(new_phys);

        memset(table, 0, PAGE_SIZE);

        *entry = new_phys | TABLE_FLAGS(flags);

        created_entries[created] = entry;
        created_phys[created++] = new_phys;

    } else {

        *entry |= PAGE_RW;

        if (flags & PAGE_USER)
            *entry |= PAGE_USER;
    }


    uint64_t *pd =
        phys_to_virt(*entry & PAGE_ADDR);


    /* PD -> PT */

    entry = &pd[pdi];

    if (*entry & PAGE_PS)
        goto fail;

    if (!(*entry & PAGE_PRESENT)) {

        uint64_t new_phys = bitmap_alloc(1);

        if (!new_phys)
            goto fail;

        uint64_t *table = phys_to_virt(new_phys);

        memset(table, 0, PAGE_SIZE);

        *entry = new_phys | TABLE_FLAGS(flags);

        created_entries[created] = entry;
        created_phys[created++] = new_phys;

    } else {

        *entry |= PAGE_RW;

        if (flags & PAGE_USER)
            *entry |= PAGE_USER;
    }


    uint64_t *pt =
        phys_to_virt(*entry & PAGE_ADDR);


    /* PT -> PAGE */

    if (pt[pti] & PAGE_PRESENT)
        goto fail;

    pt[pti] =
        (phys & PAGE_ADDR) |
        LEAF_FLAGS(flags) |
        PAGE_PRESENT;


    asm volatile(
        "invlpg (%0)"
        :
        : "r"(virt)
        : "memory"
    );

    return 0;


fail:

    rollback_tables(
        created_entries,
        created_phys,
        created
    );

    return -1;
}


/* =========================================================
   MAP RANGE
   ========================================================= */

int map_range_generic(
    uint64_t *generic_pml4,
    uint64_t virt,
    uint64_t phys,
    uint64_t flags,
    size_t size
)
{
    if (!generic_pml4)
        return -1;

    if ((virt & 0xFFF) || (phys & 0xFFF))
        return -1;

    if (size == 0)
        return 0;

    if (size > SIZE_MAX - 4095)
        return -1;

    size_t aligned =
        (size + 4095) & ~(size_t)4095;

    if (aligned - PAGE_SIZE > UINT64_MAX - virt)
        return -1;

    if (aligned - PAGE_SIZE > UINT64_MAX - phys)
        return -1;

    size_t mapped = 0;

    for (size_t i = 0; i < aligned; i += PAGE_SIZE) {

        if (map_generic(
                generic_pml4,
                virt + i,
                phys + i,
                flags
            ) != 0) {

            for (size_t j = 0; j < mapped; j += PAGE_SIZE)
                unmap_generic(generic_pml4, virt + j);

            return -1;
        }

        mapped += PAGE_SIZE;
    }

    return 0;
}


/* =========================================================
   GET PHYSICAL ADDRESS
   ========================================================= */

uint64_t get_phys_generic(
    uint64_t *generic_pml4,
    uint64_t virt
)
{
    if (!generic_pml4 || !is_canonical(virt))
        return UINT64_MAX;

    uint64_t entry;

    entry = generic_pml4[PML4_INDEX(virt)];

    if (!(entry & PAGE_PRESENT))
        return UINT64_MAX;

    uint64_t *pdpt =
        phys_to_virt(entry & PAGE_ADDR);


    entry = pdpt[PDPT_INDEX(virt)];

    if (!(entry & PAGE_PRESENT))
        return UINT64_MAX;


    /* 1 GiB page */

    if (entry & PAGE_PS) {

        return (entry & 0x000FFFFFC0000000ULL) |
               (virt & 0x3FFFFFFFULL);
    }


    uint64_t *pd =
        phys_to_virt(entry & PAGE_ADDR);


    entry = pd[PD_INDEX(virt)];

    if (!(entry & PAGE_PRESENT))
        return UINT64_MAX;


    /* 2 MiB page */

    if (entry & PAGE_PS) {

        return (entry & 0x000FFFFFFFE00000ULL) |
               (virt & 0x1FFFFFULL);
    }


    uint64_t *pt =
        phys_to_virt(entry & PAGE_ADDR);


    entry = pt[PT_INDEX(virt)];

    if (!(entry & PAGE_PRESENT))
        return UINT64_MAX;


    return (entry & PAGE_ADDR) |
           (virt & 0xFFF);
}


/* =========================================================
   IS MAPPED
   ========================================================= */

bool is_mapped(
    uint64_t *generic_pml4,
    uint64_t virt_addr
)
{
    return get_phys_generic(
        generic_pml4,
        virt_addr
    ) != UINT64_MAX;
}


/* =========================================================
   UNMAP GENERIC
   ========================================================= */

int unmap_generic(
    uint64_t *generic_pml4,
    uint64_t virt
)
{
    uint64_t *leaf;

    if (walk_4k(generic_pml4, virt, &leaf) != 0)
        return -1;

    *leaf = 0;

    asm volatile(
        "invlpg (%0)"
        :
        : "r"(virt)
        : "memory"
    );

    return 0;
}


/* =========================================================
   UNMAP RANGE
   ========================================================= */

int unmap_range_generic(
    uint64_t *generic_pml4,
    uint64_t virt,
    size_t size
)
{
    if (!generic_pml4 || (virt & 0xFFF))
        return -1;

    if (size == 0)
        return 0;

    if (size > SIZE_MAX - 4095)
        return -1;

    size_t aligned =
        (size + 4095) & ~(size_t)4095;

    if (aligned - PAGE_SIZE > UINT64_MAX - virt)
        return -1;


    /* Verifica todas as páginas antes de remover. */

    for (size_t i = 0; i < aligned; i += PAGE_SIZE) {

        uint64_t *leaf;

        if (walk_4k(
                generic_pml4,
                virt + i,
                &leaf
            ) != 0) {

            return -1;
        }
    }


    /* Remove os mapeamentos. */

    for (size_t i = 0; i < aligned; i += PAGE_SIZE) {

        if (unmap_generic(
                generic_pml4,
                virt + i
            ) != 0) {

            return -1;
        }
    }

    return 0;
}

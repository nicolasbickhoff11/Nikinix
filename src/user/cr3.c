#include "map.h"
#include "pmm.h"
#include "string.h"

uint64_t *setup_user_cr3() {

    // 1. aloca PML4 (4KB PAGE)
    uint64_t *user_pml4 =
        (uint64_t*)phys_to_virt(bitmap_alloc(1));

    map_generic(pml4, user_pml4, (uint64_t)virt_to_phys((uint64_t)user_pml4), PAGE_PRESENT | PAGE_RW);

    memset(user_pml4, 0, 4096);

    // 2. copia apenas KERNEL HALF (256–511)
    for (int i = 256; i < 512; i++) {
        user_pml4[i] = pml4[i];
    }

    return user_pml4;
}

#include "vmm.h"
#include "pmm.h"
#include "print.h"
#include "map.h"
#include <stdint.h>
#include <stddef.h>

void *virtual_alloc(size_t blocks) {
    uint64_t base = bitmap_alloc(blocks);
    if (base == UINT64_MAX) return NULL;
    return phys_to_virt(base);
}

int virtual_free(void *base, size_t blocks) {
    return bitmap_free(virt_to_phys(base), blocks);
}

#include "vmm.h"
#include "pmm.h"
#include "print.h"
#include "map.h"

#include <stdint.h>
#include <stddef.h>

void *virtual_alloc(size_t blocks)
{
    if (blocks == 0)
        return NULL;

    uint64_t base = bitmap_alloc(blocks);

    if (base == UINT64_MAX)
        return NULL;

    void *virt = phys_to_virt(base);

    if (!virt) {
        bitmap_free(base, blocks);
        return NULL;
    }

    return virt;
}

int virtual_free(void *base, size_t blocks)
{
    if (!base || blocks == 0)
        return -1;

    uint64_t phys = virt_to_phys(base);

    if (phys == UINT64_MAX)
        return -1;

    return bitmap_free(phys, blocks);
}

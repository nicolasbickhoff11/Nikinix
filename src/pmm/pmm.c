#include "print.h"
#include "pmm.h"
#include "map.h"
#include <stdint.h>
#include "limine.h"

uint64_t heap = 0;
uint64_t heap_end = 0;

uint64_t bump_alloc() {
    //if (heap >= heap_end) return 0;
    uint64_t page = heap;
    heap += 4096;
    kprintf("PAGE: 0x%lx\n", page);
    return page;
}

extern LIMINE_PTR(struct limine_memmap_response *) memmap_ptr;

struct bitmap_struct bitmap = {0};

void pmm_init() {
    //heap = (mem_avaliable[1].base + 4095) & ~4095;
    //heap_end = (mem_avaliable[1].base + mem_avaliable[1].length + 4095) & ~4095;

    uint64_t page_count = ram_total / 4096;
    bitmap.bitmapSizeinBytes = (page_count + 7) / 8;
    bitmap.bitmapAllocSizeinBytes = (bitmap.bitmapSizeinBytes + 4095) & ~4095;
    kprintf("[PMM] Bitmap Alloc Size (bytes): %ld\n", bitmap.bitmapAllocSizeinBytes);
    for (uint64_t i = 0; i < memmap_ptr->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_ptr->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE || entry->length < bitmap.bitmapAllocSizeinBytes) continue;
        bitmap.bitmap_phys = (uint64_t)(entry->base + 4095) & ~4095;
        bitmap.bitmap_virt = (uint8_t*)phys_to_virt(bitmap.bitmap_phys);
        break;
    }

    memset(bitmap.bitmap_virt, 0xFF, bitmap.bitmapAllocSizeinBytes);

    for (uint64_t i = 0; i < memmap_ptr->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_ptr->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE) continue;
        uint64_t size = (entry->length + 4095) & ~4095;
        uint64_t base = (entry->base + 4095) & ~4095;
        uint64_t page_c = 0;
        for (page_c = base; page_c < (base + size); page_c += 4096) {
            if ((page_c >= bitmap.bitmap_phys) && (page_c < bitmap.bitmap_phys + bitmap.bitmapAllocSizeinBytes)) continue;
            ClearPage(page_c);
        }
    }
}

int is_reserved(uint64_t page) {
    for (uint64_t i = 0; i < memmap_ptr->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_ptr->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE) {
            uint64_t size = (entry->length + 4095) & ~4095;
            uint64_t base = (entry->base + 4095) & ~4095;
            uint64_t page_c = 0;
            for (page_c = base; page_c < (base + size); page_c += 4096) {
                if (page == page_c || (page >= page_c && page < page_c + 4096)) return 1;
            }
        }
        if (page >= bitmap.bitmap_phys && page < bitmap.bitmap_phys + bitmap.bitmapAllocSizeinBytes) return 1;
        if (!page) return 1;
    }
    return 0;
}

size_t FindFreeRegion(size_t blocks)
{
    size_t totalBits = bitmap.bitmapSizeinBytes * 8;
    size_t start = bitmap.lastIndex * 8 + bitmap.lastBit;

    size_t regionStart = 0;
    size_t regionSize = 0;

    for (size_t bit = start; bit < totalBits; bit++) {
        size_t byte = bit / 8;
        size_t index = bit % 8;

        if (!GetBit(byte, index)) {
            if (regionSize == 0)
                regionStart = bit;

            regionSize++;

            if (regionSize == blocks) {
                bitmap.lastIndex = (regionStart + blocks) / 8;
                bitmap.lastBit   = (regionStart + blocks) % 8;

                return regionStart;
            }
        } else {
            regionSize = 0;
        }
    }

    regionSize = 0;

    for (size_t bit = 0; bit < start; bit++) {
        size_t byte = bit / 8;
        size_t index = bit % 8;

        if (!GetBit(byte, index)) {
            if (regionSize == 0)
                regionStart = bit;

            regionSize++;

            if (regionSize == blocks) {
                bitmap.lastIndex = (regionStart + blocks) / 8;
                bitmap.lastBit   = (regionStart + blocks) % 8;

                return regionStart;
            }
        } else {
            regionSize = 0;
        }
    }

    return SIZE_MAX;
}

uint64_t bitmap_alloc(size_t blocks) {
    uint64_t pos = FindFreeRegion(blocks);
    if (pos == SIZE_MAX) return UINT64_MAX;
    SetRegion(pos * 4096, blocks);
    return pos * 4096;
}

int bitmap_free(uint64_t base, size_t blocks) {
    if (is_reserved(base)) { kprintf("[ERROR] Page Reserved!\n"); return -1; }
    if (is_region_free(base, blocks)) { kprintf("[ERROR] Double Free!\n"); return -2; }
    CleanRegion(base, blocks);
    return 0;
}

void SetRegion(uint64_t base, size_t blocks) {
    for (uint64_t i = base; i < base + (blocks * 4096); i += 4096) {
        SetPage(i);
    }
}

int is_region_free(uint64_t base, size_t blocks) {
    int i = 0;
    for (uint64_t j = base; j < base + (blocks * 4096); j += 4096) {
        uint64_t page = j / 4096;
        if (!GetBit(page / 8, page % 8)) i++;
    }
    if (i >= blocks) return 1;
    return 0;
}

void CleanRegion(uint64_t base, size_t blocks) {
    for (uint64_t i = base; i < base + (blocks * 4096); i += 4096) {
        ClearPage(i);
    }
}

void SetPage(uint64_t phys) {
    uint64_t page = phys / 4096;
    SetBit(page / 8, page % 8);
}

void ClearPage(uint64_t phys) {
    uint64_t page = phys / 4096;
    ClearBit(page / 8, page % 8);
}

void ClearBit(size_t index, uint8_t bit) {
    bitmap.bitmap_virt[index] &= ~(1u << bit);
}

uint8_t GetBit(size_t index, uint8_t bit)
{
    return (bitmap.bitmap_virt[index] >> bit) & 1u;
}

void SetBit(size_t index, uint8_t bit) {
    bitmap.bitmap_virt[index] |= (1u << bit);
}
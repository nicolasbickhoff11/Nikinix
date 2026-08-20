#ifndef _MAP_H
#define _MAP_H 1

#include <stdint.h>
#include <stddef.h>

#include "limine.h"

#define PAGE_PRESENT (1 << 0)
#define PAGE_RW      (1 << 1)
#define PAGE_USER    (1 << 2)
#define PAGE_PWT     (1 << 3)
#define PAGE_PCD     (1 << 4)
#define PAGE_NX      (1 << 63);

extern uint64_t *pml4;
extern uint64_t mem_from_alloc;
extern uint64_t mem_end;
extern uint64_t mem_count;
extern uint64_t hhdm_offset;

extern uint64_t kernel_text_start;
extern uint64_t kernel_text_end;

extern uint64_t kernel_rodata_start;
extern uint64_t kernel_rodata_end;

extern uint64_t kernel_data_start;
extern uint64_t kernel_data_end;

extern uint64_t kernel_bss_start;
extern uint64_t kernel_bss_end;

typedef struct {
    uint64_t base;
    uint64_t length;
} mem_t;

extern mem_t    mem_avaliable[8192];
extern uint64_t mem_count;
extern uint64_t ram_size_usable;
extern uint64_t ram_total;
extern LIMINE_PTR(struct limine_memmap_entry **) memmap;

void *phys_to_virt(uint64_t phys);
uint64_t virt_to_phys(void *virt);
int map_generic(uint64_t *generic_pml4, uint64_t virt, uint64_t phys, uint64_t flags);
int map_range_generic(uint64_t *generic_pml4, uint64_t virt, uint64_t phys, uint64_t flags, size_t size);
int unmap_range_generic(uint64_t *generic_pml4, uint64_t virt, size_t size);
int unmap_generic(uint64_t *user_pml4, uint64_t virt);
uint64_t get_phys_generic(uint64_t *generic_pml4, uint64_t virt);

#define ALIGN_DOWN(value, align) ((value) & ~((align) - 1))
#define ALIGN_UP(value, align)   (((value) + (align) - 1) & ~((align) - 1))

#endif

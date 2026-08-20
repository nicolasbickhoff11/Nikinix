#ifndef _PMM_H
#define _PMM_H 1

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

extern uint64_t heap;
extern uint64_t heap_end;

struct bitmap_struct {
    uint64_t bitmapSizeinBytes;
    uint64_t bitmapAllocSizeinBytes;
    uint8_t *bitmap_virt;
    uint64_t bitmap_phys;
    size_t lastIndex;
    uint8_t lastBit;
};

extern struct bitmap_struct bitmap;

uint64_t bump_alloc();
int is_reserved(uint64_t page);
uint64_t bitmap_alloc(size_t blocks);
int bitmap_free(uint64_t phys, size_t blocks);
void SetPage(uint64_t phys);
void ClearPage(uint64_t phys) ;
void ClearBit(size_t index, uint8_t bit);
uint8_t GetBit(size_t index, uint8_t bit);
void SetBit(size_t index, uint8_t bit);

#endif
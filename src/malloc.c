#include <stdint.h>
#include "vmm.h"
#include "pmm.h"
#include "print.h"
#include <stddef.h>

#define PAGE_SIZE 4096
#define MAX_ALLOCS 8192

#define DivRoundUp(x, y) (((x) + (y) - 1) / (y))

struct alloc {
    size_t size;
    size_t pages;
    void *p;
};

static struct alloc maps[MAX_ALLOCS] = { 0 };
static size_t total_maps = 0;

void *malloc(size_t size)
{
    if (size == 0)
        return NULL;

    if (total_maps >= MAX_ALLOCS)
        return NULL;

    size_t pages = DivRoundUp(size, PAGE_SIZE);

    void *p = virtual_alloc(pages);

    if (p == NULL)
        return NULL;

    //kprintf("%p\n", p);

    maps[total_maps].size = size;
    maps[total_maps].pages = pages;
    maps[total_maps].p = p;

    total_maps++;

    return p;
}

void *realloc(void *p, size_t new_size)
{
    if (p == NULL)
        return malloc(new_size);

    if (new_size == 0) {
        free(p);
        return NULL;
    }

    size_t old_size = 0;

    for (size_t i = 0; i < total_maps; i++) {
        if (maps[i].p == p) {
            old_size = maps[i].size;
            break;
        }
    }

    if (old_size == 0)
        return NULL;

    void *new_p = malloc(new_size);

    if (new_p == NULL)
        return NULL;

    size_t copy_size = old_size < new_size ? old_size : new_size;

    memcpy(new_p, p, copy_size);
    free(p);

    return new_p;
}

void free(void *p)
{
    if (p == NULL)
        return;

    for (size_t i = 0; i < total_maps; i++) {

        if (maps[i].p != p)
            continue;

        size_t pages = maps[i].pages;

        int r = virtual_free(p, pages);

        if (r < 0)
            return;

        /*
         * Move a última entrada para o lugar
         * da entrada que foi liberada.
         */
        total_maps--;

        if (i != total_maps)
            maps[i] = maps[total_maps];

        maps[total_maps].p = NULL;
        maps[total_maps].size = 0;
        maps[total_maps].pages = 0;

        return;
    }

    /*
     * Ponteiro não encontrado.
     * Não chama virtual_free().
     */
}

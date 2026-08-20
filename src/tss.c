#include <stdint.h>
#include "tss.h"

#include "map.h"
#include "pmm.h"

struct tss_entry_struct tss = {0};
uint8_t *kernel_int_stack = 0;

void setup_tss() {
    kernel_int_stack = (uint8_t*)phys_to_virt((uint64_t)bitmap_alloc(4));
    tss.rsp0 = (uint64_t)kernel_int_stack;
}
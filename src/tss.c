
#include <stdint.h>
#include "tss.h"
#include "map.h"
#include "pmm.h"

struct tss_entry_struct tss = {0};
uint8_t *kernel_int_stack = 0;

void setup_tss(void)
{
    uint64_t stack_phys = bitmap_alloc(4);

    if (stack_phys == 0) {
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    }

    // Quatro páginas: 4 * 4096 bytes.
    // A stack cresce para baixo, então usamos o topo.
    kernel_int_stack =
        (uint8_t *)phys_to_virt(stack_phys + 4 * 4096);

    tss.rsp0 = (uint64_t)kernel_int_stack;
}

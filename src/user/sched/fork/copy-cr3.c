
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "map.h"
#include "pmm.h"
#include "print.h"

#include "user/sched/fork/copy-cr3.h"

int clone_address_space(
    uint64_t *parent_pml4,
    uint64_t *child_pml4
) {
    if (!parent_pml4 || !child_pml4)
        return -1;

    /*
     * A PML4 do filho já foi criada por setup_user_cr3().
     * As entradas do kernel já estão configuradas.
     *
     * Não zeramos a PML4!
     */

    for (uint64_t i = 0; i < USER_PML4_ENTRIES; i++) {

        uint64_t pml4e = parent_pml4[i];

        if (!(pml4e & PAGE_PRESENT))
            continue;

        if (child_pml4[i] & PAGE_PRESENT)
            return -1;

        uint64_t *pdpt =
            phys_to_virt(pml4e & PAGE_ADDR);

        for (uint64_t j = 0; j < 512; j++) {

            uint64_t pdpte = pdpt[j];

            if (!(pdpte & PAGE_PRESENT))
                continue;

            /* Páginas de 1 GiB não suportadas. */
            if (pdpte & PAGE_PS)
                return -1;

            uint64_t *pd =
                phys_to_virt(pdpte & PAGE_ADDR);

            for (uint64_t k = 0; k < 512; k++) {

                uint64_t pde = pd[k];

                if (!(pde & PAGE_PRESENT))
                    continue;

                /* Páginas de 2 MiB não suportadas. */
                if (pde & PAGE_PS)
                    return -1;

                uint64_t *pt =
                    phys_to_virt(pde & PAGE_ADDR);

                for (uint64_t l = 0; l < 512; l++) {

                    uint64_t pte = pt[l];

                    if (!(pte & PAGE_PRESENT))
                        continue;

                    /*
                     * Só clona páginas acessíveis
                     * em modo usuário.
                     */
                    bool user =
                        (pml4e & PAGE_USER) &&
                        (pdpte & PAGE_USER) &&
                        (pde & PAGE_USER) &&
                        (pte & PAGE_USER);

                    if (!user)
                        continue;

                    uint64_t virt =
                        (i << 39) |
                        (j << 30) |
                        (k << 21) |
                        (l << 12);

                    /*
                     * Canonicaliza endereços
                     * da região superior.
                     */
                    if (virt & (1ULL << 47))
                        virt |= 0xFFFF000000000000ULL;

                    uint64_t phys = pte & PAGE_ADDR;

                    uint64_t flags = pte &
                        (PAGE_RW |
                         PAGE_USER |
                         PAGE_PWT |
                         PAGE_PCD |
                         PAGE_NX);

                    /*
                     * Aloca frame privado para o filho.
                     */
                    uint64_t new_phys = bitmap_alloc(1);

                    if (!new_phys)
                        return -1;

                    memcpy(
                        phys_to_virt(new_phys),
                        phys_to_virt(phys),
                        PAGE_SIZE
                    );

                    /*
                     * Cria o mapeamento no filho.
                     */
                    if (map_generic(
                            child_pml4,
                            virt,
                            new_phys,
                            flags
                        ) != 0) {

                        bitmap_free(new_phys, 1);
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}

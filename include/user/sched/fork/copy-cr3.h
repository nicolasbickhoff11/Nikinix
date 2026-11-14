#ifndef _USER_SCHED_FORK_COPY_CR3
#define _USER_SCHED_FORK_COPY_CR3 1

#include <stdint.h>

#define PML4_ENTRIES 512
#define USER_PML4_ENTRIES 256

int clone_address_space(
    uint64_t *parent_pml4,
    uint64_t *child_pml4
);

#endif
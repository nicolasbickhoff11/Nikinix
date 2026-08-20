#ifndef _VMM_H
#define _VMM_H 1

#include <stddef.h>

void *virtual_alloc(size_t blocks);
int virtual_free(void *base, size_t blocks);

#endif
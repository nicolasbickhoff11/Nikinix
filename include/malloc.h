#ifndef _MALLOC_H
#define _MALLOC_H 1

#include <stddef.h>

void *malloc(size_t size);
void *realloc(void *p, size_t new_size);
void free(void *p);

#endif
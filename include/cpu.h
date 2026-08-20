#ifndef _CPU_H
#define _CPU_H 1

#include <stdint.h>

void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
void cpu_name(char *buf);

#endif

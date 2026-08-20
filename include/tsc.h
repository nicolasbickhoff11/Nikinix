#ifndef _TSC_H
#define _TSC_H 1

#include <stdint.h>

static inline uint64_t rdtsc(void) {
    uint32_t low, high;
    __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

extern uint64_t tsc_frequency;

#endif

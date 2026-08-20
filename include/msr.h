#ifndef _MSR_H
#define _MSR_H 1

#include <stdint.h>

#define IA32_EFER 0xC0000080
#define EFER_SCE (1 << 0)
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    // 'c' maps to ECX, 'a' to EAX, 'd' to EDX
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = (uint32_t)(value & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(value >> 32);

    /*
     * "c" -> Register ECX (the MSR address)
     * "a" -> Register EAX (the low 32 bits)
     * "d" -> Register EDX (the high 32 bits)
     * 
     * volatile prevents the compiler from optimizing the instruction away.
     */
    __asm__ volatile (
        "wrmsr"
        : 
        : "c" (msr), "a" (low), "d" (high)
        : "memory"
    );
}

#endif
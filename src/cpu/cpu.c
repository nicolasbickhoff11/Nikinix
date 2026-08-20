#include <stdint.h>

void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile (
        "cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (leaf), "c" (subleaf)
    );
}

void cpu_name(char *buf) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(0x80000002, 0x00, &eax, &ebx, &ecx, &edx);
    uint32_t *p = (uint32_t*)buf;
    p[0] = eax;
    p[1] = ebx;
    p[2] = ecx;
    p[3] = edx;
    cpuid(0x80000003, 0x00, &eax, &ebx, &ecx, &edx);
    p[4] = eax;
    p[5] = ebx;
    p[6] = ecx;
    p[7] = edx;
    cpuid(0x80000004, 0x00, &eax, &ebx, &ecx, &edx);
    p[8] = eax;
    p[9] = ebx;
    p[10] = ecx;
    p[11] = edx;
    buf[12 * sizeof(uint32_t)] = '\0';
}

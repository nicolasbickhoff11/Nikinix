#include "msr.h"
#include "syscall/main.h"
#include "syscall/enable.h"
#include <stdint.h>

extern void syscall_entry();

void setup_syscall() {
    uint64_t efer = rdmsr(IA32_EFER);
    efer |= EFER_SCE;
    wrmsr(IA32_EFER, efer);
    uint64_t star = rdmsr(IA32_STAR);
    star &= ~0xFFFFFFFFFFFFFFFFULL;
    star |= ((uint64_t)0x18 << 48) | ((uint64_t)0x08 << 32);
    wrmsr(IA32_STAR, star);
    wrmsr(IA32_LSTAR, (uint64_t)syscall_entry);
    wrmsr(IA32_FMASK, (1 << 9) | (1 << 8));
    syscall_table[_NR_write] = _x64_sys_write;
    syscall_table[_NR_read] = _x64_sys_read;
    syscall_table[_NR_sleep] = _x64_sys_sleep;
}
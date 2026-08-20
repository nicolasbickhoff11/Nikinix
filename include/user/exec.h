#ifndef _USER_EXEC_H
#define _USER_EXEC_H 1

#include <stdint.h>

uint64_t load_elf(uint64_t *user_cr3, const char *path);

#endif
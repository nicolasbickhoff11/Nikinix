#include <stdint.h>
#include "print.h"

void _putchar(char character) {
    print_char(character);
}

#define printf kprintf

#define PRINTF_SUPPORT_FLOAT 0
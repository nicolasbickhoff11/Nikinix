#include "acpi/checksum.h"
#include <stdint.h>

uint8_t check_checksum(uint8_t *bytes, uint32_t length) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; i++) {
        sum += bytes[i];
    }

    return sum;
}

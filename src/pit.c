#include "io.h"
#include "pit.h"
#include <stdint.h>

void PitInit() {
    uint32_t hz = 1000;
    uint32_t div = PIT_FREQUENCY / hz;
    outb(PIT_CMD, CMD_BINARY | CMD_MODE3 | CMD_RW_BOTH | CMD_COUNTER0);
    outb(PIT_COUNTER0, div);
    outb(PIT_COUNTER0, div >> 8);
}

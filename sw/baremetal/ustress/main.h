#include <stdint.h>
#include "common.h"

#ifndef LOOPS
#define LOOPS 1000
#endif

void stress(long runs);

void main() {
    volatile uint32_t loops = LOOPS;
    stress(loops);
    pass();
}

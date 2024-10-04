#include <stdint.h>
#include "common.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#ifndef N_IN
#define N_IN 10
#endif

#if (N_IN == 20)
#define SET_N volatile uint32_t n = 20;
#define SET_EXP uint64_t expected = 2432902008176640000ULL;
#elif (N_IN == 17)
#define SET_N volatile uint32_t n = 17;
#define SET_EXP uint64_t expected = 355687428096000ULL;
#elif (N_IN == 10)
#define SET_N volatile uint32_t n = 10;
#define SET_EXP uint64_t expected = 3628800;
#endif

uint64_t factorial(uint32_t n) {
    if (n == 0)
        return 1;
    return n * factorial(n-1);
}

void main() {
    SET_N
    SET_EXP
    for (uint32_t i = 0; i < LOOPS; i++) {
        uint64_t result = factorial(n);

        if (result != expected){
            write_mismatch(result>>32, result & 0xFFFFFFFF, 1);
            fail();
        }
    }
    pass();
}

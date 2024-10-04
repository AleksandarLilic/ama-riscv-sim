#include <stdint.h>
#include "common.h"
#include "func.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#ifndef N_LARGE
#define N_LARGE
#endif

// TODO: Add more test cases
#ifdef N_LARGE
#define SET_A volatile uint32_t a = 10440125;
#define SET_B volatile uint32_t b = 157216;
//#define EXPECTED_GCD 4913
#define SET_EXP uint32_t expected = 334084000;
#endif

void main() {
    SET_A
    SET_B
    SET_EXP
    for (uint32_t i = 0; i < LOOPS; i++) {
        uint64_t result = lcm(a, b);
        if (result != expected){
            write_mismatch(result, expected, 1);
            fail();
        }
    }
    pass();
}

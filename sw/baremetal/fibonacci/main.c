#include <stdint.h>
#include "common.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#ifndef N_IN
#define N_IN 10
#endif

#if (N_IN == 40)
#define SET_N volatile uint32_t n = 40;
#define SET_EXP uint32_t expected = 102334155;
#elif (N_IN == 30)
#define SET_N volatile uint32_t n = 30;
#define SET_EXP uint32_t expected = 832040;
#elif (N_IN == 20)
#define SET_N volatile uint32_t n = 20;
#define SET_EXP uint32_t expected = 6765;
#elif (N_IN == 18)
#define SET_N volatile uint32_t n = 18;
#define SET_EXP uint32_t expected = 2584;
#elif (N_IN == 15)
#define SET_N volatile uint32_t n = 15;
#define SET_EXP uint32_t expected = 610;
#elif (N_IN == 10)
#define SET_N volatile uint32_t n = 10;
#define SET_EXP uint32_t expected = 55;
#endif

/*
uint32_t fib(uint32_t n) {
    uint32_t a = 0;
    uint32_t b = 1;
    uint32_t tmp;
    while (n-- > 0) {
        tmp = a;
        a = b;
        b += tmp;
    }
    return a;
}
*/

uint32_t fib(uint32_t n) {
    if (n <= 2)
        return 1;
    else
        return fib(n-1) + fib(n-2);
}

void main() {
    SET_N
    SET_EXP
    for (uint32_t i = 0; i < LOOPS; i++) {
        LOG_START;
        uint32_t result = fib(n);
        LOG_STOP;

        if (result != expected){
            write_mismatch(result, expected, 1);
            fail();
        }
    }
    pass();
}

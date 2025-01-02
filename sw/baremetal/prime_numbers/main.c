#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#ifndef N_IN
#define N_IN 100
#endif

#if (N_IN == 3000)
#define SET_N volatile uint32_t n = 3000;
#define SET_EXP uint32_t expected = 430;
#elif (N_IN == 100)
#define SET_N volatile uint32_t n = 100;
#define SET_EXP uint32_t expected = 25;
#else
_Static_assert(0, "N_IN is not supported");
#endif

#define MAX_LIMIT 10000
_Static_assert(MAX_LIMIT >= N_IN, "MAX_LIMIT is smaller than N_IN");
#undef MAX_LIMIT
#define MAX_LIMIT N_IN

// Static memory allocation
volatile _Bool prime[MAX_LIMIT + 1];

void sieve_of_eratosthenes(volatile uint32_t n, volatile uint32_t expected) {
    asm(".global set_defaults");
    asm("set_defaults:");
    uint32_t prime_count = 0;
    for (uint32_t i = 2; i <= n; i++)
        prime[i] = true;

    asm(".global find_primes");
    asm("find_primes:");
    for (uint32_t p = 2; p * p <= n; p++)
        if (prime[p] == 1)
            for (uint32_t i = p * p; i <= n; i += p)
                prime[i] = false;

    asm(".global count_primes");
    asm("count_primes:");
    for (uint32_t p = 2; p <= n; p++)
        if (prime[p])
            prime_count++;

    if (prime_count != expected){
        write_mismatch(prime_count, expected, 1);
        fail();
    }
}

void main() {
    SET_N
    SET_EXP
    for (uint32_t i = 0; i < LOOPS; i++) {
        LOG_START;
        sieve_of_eratosthenes(n, expected);
        LOG_STOP;
    }
    pass();
}

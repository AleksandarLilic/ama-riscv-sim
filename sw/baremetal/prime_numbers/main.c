#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#define MAX_LIMIT 10000
//#define EXPECTED_PRIME_COUNT 4 // for n = 10
//#define EXPECTED_PRIME_COUNT 25 // for n = 100
#define EXPECTED_PRIME_COUNT 430 // for n = 3000

void fail();
void pass();

// Static memory allocation
volatile _Bool prime[MAX_LIMIT + 1];

void sieve_of_eratosthenes(volatile uint32_t n) {
    asm(".global set_defaults");
    asm("set_defaults:");
    uint32_t prime_count = 0;
    for (uint32_t i = 2; i <= n; i++)
        prime[i] = 1;

    asm(".global find_primes");
    asm("find_primes:");
    for (uint32_t p = 2; p * p <= n; p++)
        if (prime[p] == 1)
            for (uint32_t i = p * p; i <= n; i += p)
                prime[i] = 0;

    asm(".global count_primes");
    asm("count_primes:");
    for (uint32_t p = 2; p <= n; p++)
        if (prime[p])
            prime_count++;

    if (prime_count != EXPECTED_PRIME_COUNT){
        write_mismatch(prime_count, EXPECTED_PRIME_COUNT, 0);
        fail();
    }
}

void main() {
    uint32_t n = 3000;
    sieve_of_eratosthenes(n);
    pass();
}

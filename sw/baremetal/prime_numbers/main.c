#define MAX_LIMIT 10000
//#define EXPECTED_PRIME_COUNT 4 // for n = 10
//#define EXPECTED_PRIME_COUNT 25 // for n = 100
#define EXPECTED_PRIME_COUNT 430 // for n = 3000

void fail();
void pass();

// Static memory allocation
volatile char prime[MAX_LIMIT + 1];

void sieve_of_eratosthenes(volatile int n) {
    asm(".global set_defaults");
    asm("set_defaults:");
    unsigned int prime_count = 0;
    for (int i = 2; i <= n; i++)
        prime[i] = 1;

    asm(".global find_primes");
    asm("find_primes:");
    for (int p = 2; p * p <= n; p++)
        if (prime[p] == 1)
            for (int i = p * p; i <= n; i += p)
                prime[i] = 0;

    asm(".global count_primes");
    asm("count_primes:");
    for (int p = 2; p <= n; p++)
        if (prime[p])
            prime_count++;

    asm volatile("add x29, x0, %0"
                  :
                  : "r"(EXPECTED_PRIME_COUNT));
    asm volatile("add x30, x0, %0"
                  :
                  : "r"(prime_count));
    if (prime_count != EXPECTED_PRIME_COUNT){
        asm volatile("addi x28, x0, 1"
                      :
                      : );
        fail();
    }
}

int main() {
    int n = 3000;
    sieve_of_eratosthenes(n);
    pass();
}

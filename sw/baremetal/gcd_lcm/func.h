#include <stdint.h>

uint64_t gcd(uint32_t a, uint32_t b) {
    if (b == 0)
        return a;
    return gcd(b, a % b);
}

uint64_t lcm(uint32_t a, uint32_t b) {
    return (a / gcd(a, b)) * b;
}

#include <stdint.h>
#include "common.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#define ARR_LEN 16

volatile uint8_t a[ARR_LEN] = {
    99, 228, 216, 184, 167, 203, 173, 107, 177, 205, 12, 128, 51, 95, 222, 212};
volatile uint8_t b[ARR_LEN] = {
    152, 237, 93, 250, 200, 154, 185, 76, 1, 93, 58, 80, 88, 124, 125, 251};
uint32_t c[ARR_LEN] = {0};

const uint32_t ref[ARR_LEN] = {
    51456, 20224, 29952, 6400, 7168, 32768, 17920, 81920, 6656,
    157184, 304384, 534784, 1087488, 2159616, 4223744, 8420608};

void set_c() {
    for (uint8_t i = 0; i < ARR_LEN; i++)
        c[i] = 0;
}

void main(void) {
    uint32_t ac1, ac2;
    for (uint32_t i = 0; i < LOOPS; i++) {
        set_c();
        for (uint8_t j = 0; j < 64; j++) {
            for (uint8_t k = 0; k < ARR_LEN; k++) {
                ac1 = (a[k] && b[k]>>2)<<k;
                ac2 = ((a[k] ^ 0xAA) - (b[k] || 0x2F));
                c[k] += (ac1 + ac2) << 2;
            }
        }

        for (uint8_t j = 0; j < ARR_LEN; j++) {
            if (c[j] != ref[j]) {
                write_mismatch(c[j], ref[j], j+1); // +1 to avoid writing 0
                fail();
            }
        }
    }
    pass();
}

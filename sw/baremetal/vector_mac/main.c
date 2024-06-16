#include <stdint.h>
#include "common.h"

#define ARR_LEN 16

volatile uint8_t a[ARR_LEN] = {
    99, 228, 216, 184, 167, 203, 173, 107, 177, 205, 12, 128, 51, 95, 222, 212};
volatile uint8_t b[ARR_LEN] = {
    152, 237, 93, 250, 200, 154, 185, 76, 1, 93, 58, 80, 88, 124, 125, 251};
uint32_t c[ARR_LEN] = {0};

const uint32_t ref[ARR_LEN] = {
    963072, 3458304, 1285632, 2944000, 2137600, 2000768, 2048320, 520448,
    11328, 1220160, 44544, 655360, 287232, 753920, 1776000, 3405568
};

int asm_add(uint32_t a, uint32_t b) {
    asm volatile("add %0, %1, %2"
                 : "=r"(a)
                 : "r"(a), "r"(b));
    return a;
}

void set_c() {
    for (uint8_t i = 0; i < ARR_LEN; i++)
        c[i] = 0;
}

void main(void) {
    for (uint32_t i = 0; i < LOOPS; i++) {
        set_c();
        for (uint8_t j = 0; j < 64; j++) {
            for (uint8_t k = 0; k < ARR_LEN; k++) {
                c[k] = asm_add(c[k], a[k] * b[k]);
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

#include <stdint.h>

#define LOOP_COUNT 1u

void fail();
void pass();

uint8_t a[16] = {
    99, 228, 216, 184, 167, 203, 173, 107, 177, 205, 12, 128, 51, 95, 222, 212};
uint8_t b[16] = {
    152, 237, 93, 250, 200, 154, 185, 76, 1, 93, 58, 80, 88, 124, 125, 251};
uint32_t c[16] = {0};

const uint32_t ref[16] = {
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
    for (uint8_t i = 0; i < 16; i++) {
        c[i] = 0;
    }
}

void write_mismatch(uint32_t c, uint32_t ref, uint8_t j) {
    asm volatile("add x29, x0, %0"
                 :
                 : "r"(c));
    asm volatile("add x30, x0, %0"
                 :
                 : "r"(ref));
    asm volatile("add x28, x0, %0"
                 :
                 : "r"(j+1)); // so that 0th index is 1st
}

void main(void) {
    for (uint32_t i = 0; i < LOOP_COUNT; i++) {
        set_c();
        for (uint8_t j = 0; j < 64; j++) {
            for (uint8_t k = 0; k < 16; k++) {
                c[k] = asm_add(c[k], a[k] * b[k]);
            }
        }

        for (uint8_t j = 0; j < 16; j++) {
            if (c[j] != ref[j]) {
                write_mismatch(c[j], ref[j], j);
                fail();
            }
        }
    }
    pass();
}

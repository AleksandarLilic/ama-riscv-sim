#include <stdint.h>
#include "common.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#define ARR_LEN 16

int8_t a[ARR_LEN] = {
    44, -81, -11, 64, -61, 123, 67, -25, -119, 83, -107, 114, -92, -41, -58, 88
};
int8_t b[ARR_LEN] = {
    -40, 12, -70, 65, 102, -89, -41, 46, -40, -47, 37, -103, -51, -56, -119, 20
};
const int32_t ref = -18060;

#ifdef CUSTOM_ISA
int32_t fma8(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("fma8 %0, %1, %2"
                 : "=r"(c)
                 : "r"(a), "r"(b));
    return c;
}
#endif

int32_t mac(int8_t *a, int8_t *b, size_t len) {
    int32_t c = 0;

    #if defined(LOAD_OPT) || defined(CUSTOM_ISA)
    for (size_t k = 0; k < len; k += 4) {
        int32_t a_slice = *(int32_t*)(a + k);
        int32_t b_slice = *(int32_t*)(b + k);

        #ifdef CUSTOM_ISA
        c += fma8(a_slice, b_slice);

        #else
        // Loop through each byte in the 32-bit slice
        for (size_t i = 0; i < 4; i++) {
            int8_t a_byte = (a_slice >> (i << 3)) & 0xFF;
            int8_t b_byte = (b_slice >> (i << 3)) & 0xFF;
            c += a_byte * b_byte;
        }
        #endif
    }

    #else
    // generic implementation
    for (size_t k = 0; k < len; k++) {
        c += a[k] * b[k];
    }
    #endif
    return c;
}

void main(void) {
    for (size_t i = 0; i < LOOPS; i++) {
        int32_t result = mac(a, b, ARR_LEN);
        if (result != ref) {
            write_mismatch(result, ref, 1);
            fail();
        }
    }
    pass();
}

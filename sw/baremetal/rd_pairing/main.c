#include <stdint.h>
#include "common.h"

#define ARR_LEN 16
uint16_t a[ARR_LEN] = {
    44, 81, 11, 64, 61, 123, 67, 25, 119, 83, 107, 114, 92, 41, 58, 88
};

void unpack16u(uint16_t* in, uint32_t* out, size_t len) {
    size_t len_s2 = (len >> 1) << 1;
    for (size_t k = 0; k < len_s2; k += 2) {
        uint32_t a_slice = *(uint32_t*)(in + k);
        register uint32_t b_slice_1 asm("t5");
        register uint32_t b_slice_2 asm("t6");
        asm volatile(
            "widen16u %[v1], %[in];"
            : [v1] "=r" (b_slice_1), [v2] "=r" (b_slice_2) // [v2] "+r"
            : [in] "r" (a_slice)
            :
        );
        *(out + k) = b_slice_1;
        *(out + k + 1) = b_slice_2;
    }
}

void main() {
    uint32_t out[ARR_LEN];
    PROF_START;
    unpack16u(a, out, ARR_LEN);
    PROF_STOP;
    for (size_t i = 0; i < ARR_LEN; i++) {
        if (a[i] != out[i]) {
            fail();
        }
    }
    pass();
}

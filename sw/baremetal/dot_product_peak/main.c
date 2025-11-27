#include <stdint.h>
#include "common.h"
#include "common_math.h"

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

#define ASM_BLOCK \
    asm volatile ( \
        "dot8 %[p1], %[a0], %[b0];" \
        "dot8 %[p2], %[a1], %[b1];" \
        "dot8 %[p3], %[a2], %[b2];" \
        "dot8 %[p4], %[a3], %[b3];" \
        : [c] "+r" (c), \
          [p1] "+r" (p_arr[0]), \
          [p2] "+r" (p_arr[1]), \
          [p3] "+r" (p_arr[2]), \
          [p4] "+r" (p_arr[3]) \
        : [a0] "r" (a_arr[0]), [b0] "r" (b_arr[0]), \
          [a1] "r" (a_arr[1]), [b1] "r" (b_arr[1]), \
          [a2] "r" (a_arr[2]), [b2] "r" (b_arr[2]), \
          [a3] "r" (a_arr[3]), [b3] "r" (b_arr[3]) \
        : \
    );

#define ASM_BLOCK_8 \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK;

void main(void) {
    int32_t c = 0;
    int32_t a_arr[4], b_arr[4], p_arr[4];
    #pragma GCC unroll 4
    for (size_t i = 0; i < 4; i++) {
        a_arr[i] = *(const int32_t*)(a + i * 4);
        b_arr[i] = *(const int32_t*)(b + i * 4);
    }
    PROF_START;
    for (size_t i = 0; i < LOOPS; i++) {
        ASM_BLOCK_8;
        ASM_BLOCK_8;
    }
    PROF_STOP;
    //printf("%d\n",result);
    //printf("%d\n",ref);
    pass();
}

#include <stdint.h>
#include "common.h"
#include "common_math.h"

#ifdef NF_INT8
#include "test_matrices_int8.h"

void matmul() {
    for (size_t i = 0; i < (A_ROWS >> 2); i++) {
        for (size_t j = 0; j < (B_COLS >> 2); j++) {
            size_t cj = (j << 2);
            for (size_t k = 0; k < (A_COLS_B_ROWS >> 2); k++) {

                int8x8_t bs_t16_02, bs_t16_13;
                _simd_txp_4x4_int8(B_COLS, b, k, j, &bs_t16_02, &bs_t16_13);

                int32_t c_arr[4];
                for (size_t tr = 0; tr < 4; tr++) { // tile rows
                    // load first to prevent load-to-use dependency
                    const int8x4_t as = v_load_int8x4(&a[(i<<2)+tr][k<<2]);
                    c_arr[0] = c[(i<<2) + tr][cj + 0];
                    c_arr[1] = c[(i<<2) + tr][cj + 1];
                    c_arr[2] = c[(i<<2) + tr][cj + 2];
                    c_arr[3] = c[(i<<2) + tr][cj + 3];

                    // dotp
                    asm volatile (
                        "dot8 %[c0], %[a], %[b0]\n\t" // scheduling?
                        "dot8 %[c1], %[a], %[b1]\n\t"
                        "dot8 %[c2], %[a], %[b2]\n\t"
                        "dot8 %[c3], %[a], %[b3]\n\t"
                        : [c0] "+r" (c_arr[0]),
                          [c1] "+r" (c_arr[1]),
                          [c2] "+r" (c_arr[2]),
                          [c3] "+r" (c_arr[3])
                        : [a] "r" (as),
                          [b0] "r" (bs_t16_02.w.lo),
                          [b1] "r" (bs_t16_13.w.lo),
                          [b2] "r" (bs_t16_02.w.hi),
                          [b3] "r" (bs_t16_13.w.hi)
                        :
                    );

                    // store back
                    c[(i<<2) + tr][cj + 0] = c_arr[0];
                    c[(i<<2) + tr][cj + 1] = c_arr[1];
                    c[(i<<2) + tr][cj + 2] = c_arr[2];
                    c[(i<<2) + tr][cj + 3] = c_arr[3];
                }
            }
        }
    }
}

#endif

#ifdef NF_INT16
#include "test_matrices_int16.h"

void matmul() {
    for (size_t i = 0; i < (A_ROWS >> 1); i++) {
        for (size_t j = 0; j < (B_COLS >> 1); j++) {
            size_t cj = (j << 1);
            for (size_t k = 0; k < (A_COLS_B_ROWS >> 1); k++) {

                int16x4_t bs_t16;
                _simd_txp_2x2_int16(B_COLS, b, k, j, &bs_t16);

                int32_t c_arr[2];
                for (size_t tr = 0; tr < 2; tr++) { // tile rows
                    // load first to prevent load-to-use dependency
                    const int16x2_t as = v_load_int16x2(&a[(i<<1)+tr][k<<1]);
                    c_arr[0] = c[(i<<1) + tr][cj + 0];
                    c_arr[1] = c[(i<<1) + tr][cj + 1];

                    // dotp
                    asm volatile (
                        "dot16 %[c0], %[a], %[b0]\n\t" // scheduling?
                        "dot16 %[c1], %[a], %[b1]\n\t"
                        : [c0] "+r" (c_arr[0]),
                          [c1] "+r" (c_arr[1])
                        : [a] "r" (as),
                          [b0] "r" (bs_t16.w.lo),
                          [b1] "r" (bs_t16.w.hi)
                        :
                    );

                    // store back
                    c[(i<<1) + tr][cj + 0] = c_arr[0];
                    c[(i<<1) + tr][cj + 1] = c_arr[1];
                }
            }
        }
    }
}

#endif

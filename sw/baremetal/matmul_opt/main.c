#include <stdint.h>
#include "common.h"
#include "common_math.h"

#include "test_matrices.h"

#ifndef LOOPS
#define LOOPS 1
#endif

void set_c() {
    for (size_t i = 0; i < A_ROWS; i++) {
        for (size_t j = 0; j < B_COLS; j++) c[i][j] = 0;
    }
}

void main(void) {
    for (uint32_t i = 0; i < LOOPS; i++) {
        set_c();
        PROF_START;

        asm(".global compute");
        asm("compute:");

        #if OPT_V_LOAD_B
        for (size_t j = 0; j < (B_COLS >> 2); j++) {
            for (size_t k = 0; k < (A_COLS_B_ROWS >> 2); k++) {
                // b slices
                const int8x4_t bs_0 = v_load_int8x4(&b[(k<<2) + 0][j<<2]);
                const int8x4_t bs_1 = v_load_int8x4(&b[(k<<2) + 1][j<<2]);
                const int8x4_t bs_2 = v_load_int8x4(&b[(k<<2) + 2][j<<2]);
                const int8x4_t bs_3 = v_load_int8x4(&b[(k<<2) + 3][j<<2]);

                // b transpose
                int8x8_t bs_t8_01, bs_t8_23;
                asm volatile(
                    "txp8 %0, %1, %2" : "=r"(bs_t8_01) : "r"(bs_0), "r"(bs_1)
                );

                asm volatile(
                    "txp8 %0, %1, %2" : "=r"(bs_t8_23) : "r"(bs_2), "r"(bs_3)
                );

                int8x8_t bs_t16_02, bs_t16_13;
                asm volatile(
                    "txp16 %0, %1, %2"
                    : "=r"(bs_t16_02)
                    : "r"(bs_t8_01.w.lo), "r"(bs_t8_23.w.lo)
                );

                asm volatile(
                    "txp16 %0, %1, %2"
                    : "=r"(bs_t16_13)
                    : "r"(bs_t8_01.w.hi), "r"(bs_t8_23.w.hi)
                );

                size_t cj = (j << 2);
                for (size_t i = 0; i < (A_ROWS); i++) {
                    const int8x4_t as = v_load_int8x4(&a[i][k<<2]);

                    // load first to prevent load-to-use dependency
                    int32_t c_arr[4];
                    c_arr[0] = c[i][cj + 0];
                    c_arr[1] = c[i][cj + 1];
                    c_arr[2] = c[i][cj + 2];
                    c_arr[3] = c[i][cj + 3];

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
                    c[i][cj + 0] = c_arr[0];
                    c[i][cj + 1] = c_arr[1];
                    c[i][cj + 2] = c_arr[2];
                    c[i][cj + 3] = c_arr[3];
                }
            }
        }

        #elif OPT_V_TILING
        for (size_t i = 0; i < (A_ROWS >> 2); i++) {
            for (size_t j = 0; j < (B_COLS >> 2); j++) {
                size_t cj = (j << 2);
                for (size_t k = 0; k < (A_COLS_B_ROWS >> 2); k++) {
                    // b slices
                    const int8x4_t bs_0 = v_load_int8x4(&b[(k<<2) + 0][j<<2]);
                    const int8x4_t bs_1 = v_load_int8x4(&b[(k<<2) + 1][j<<2]);
                    const int8x4_t bs_2 = v_load_int8x4(&b[(k<<2) + 2][j<<2]);
                    const int8x4_t bs_3 = v_load_int8x4(&b[(k<<2) + 3][j<<2]);

                    // b transpose
                    int8x8_t bs_t8_01, bs_t8_23;
                    asm volatile(
                        "txp8 %0, %1, %2"
                        : "=r"(bs_t8_01) : "r"(bs_0), "r"(bs_1)
                    );

                    asm volatile(
                        "txp8 %0, %1, %2"
                        : "=r"(bs_t8_23) : "r"(bs_2), "r"(bs_3)
                    );

                    int8x8_t bs_t16_02, bs_t16_13;
                    asm volatile(
                        "txp16 %0, %1, %2"
                        : "=r"(bs_t16_02)
                        : "r"(bs_t8_01.w.lo), "r"(bs_t8_23.w.lo)
                    );

                    asm volatile(
                        "txp16 %0, %1, %2"
                        : "=r"(bs_t16_13)
                        : "r"(bs_t8_01.w.hi), "r"(bs_t8_23.w.hi)
                    );

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
        #endif

        PROF_STOP;

        /*
        printf("Result of matrix multiplication (C = A * B):\n");
        for (int i = 0; i < A_ROWS; i++) {
            for (int j = 0; j < B_COLS; j++) {
                printf("%d ", c[i][j]);
            }
            printf("\n");
        }
        */

        asm(".global check");
        asm("check:");
        for (size_t i = 0; i < A_ROWS; i++) {
            for (size_t j = 0; j < B_COLS; j++) {
                if (c[i][j] != ref[i][j]) {
                    write_mismatch(c[i][j], ref[i][j], i * B_COLS + j + 1);
                    fail();
                }
            }
        }
    }
    pass();
}

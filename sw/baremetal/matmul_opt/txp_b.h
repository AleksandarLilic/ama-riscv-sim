#include <stdint.h>
#include "common.h"
#include "common_math.h"

#include "test_matrices.h"
int8_t bt[B_COLS][A_COLS_B_ROWS]; // swapped dims because B is transposed

void transpose() {
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

            v_store_int8x4(&bt[(j<<2) + 0][(k<<2)], bs_t16_02.w.lo);
            v_store_int8x4(&bt[(j<<2) + 1][(k<<2)], bs_t16_13.w.lo);
            v_store_int8x4(&bt[(j<<2) + 2][(k<<2)], bs_t16_02.w.hi);
            v_store_int8x4(&bt[(j<<2) + 3][(k<<2)], bs_t16_13.w.hi);
        }
    }
    /*
    for (size_t k = 0; k < (A_COLS_B_ROWS); k++) {
        printf("\n");
        for (size_t j = 0; j < (B_COLS); j++) {
            printf("%d, ", bt[k][j]);
        }
    }
    printf("\n");
    */
}

void matmul() {
    for (size_t i = 0; i < A_ROWS; i++) { // over A rows
        for (size_t j = 0; j < B_COLS; j++) { // over b.T rows
            #if A_COLS_B_ROWS >= 8
            int32_t c_acc = c[i][j];
            const size_t ks = 2 + 1; // step - 2 B to W load, + 1 for 2x at once
            for (size_t k = 0; k < (A_COLS_B_ROWS >> ks); k++) {
                // load first to prevent load-to-use dependency
                const int8x4_t as_0 = v_load_int8x4(&a[i][(k<<ks)]);
                const int8x4_t as_1 = v_load_int8x4(&a[i][(k<<ks)+4]);
                const int8x4_t bs_0 = v_load_int8x4(&bt[j][(k<<ks)]);
                const int8x4_t bs_1 = v_load_int8x4(&bt[j][(k<<ks)+4]);
                // dotp
                asm volatile (
                    "dot8 %[c], %[a0], %[b0]\n\t" // scheduling?
                    "dot8 %[c], %[a1], %[b1]\n\t"
                    : [c] "+r" (c_acc)
                    : [a0] "r" (as_0), [a1] "r" (as_1),
                      [b0] "r" (bs_0), [b1] "r" (bs_1)
                    :
                );
            }
            // store back
            c[i][j] = c_acc;

            #else // A_COLS_B_ROWS < 8
            c[i][j] = _simd_dot_product_int8(a[i], bt[j], A_COLS_B_ROWS);
            #endif
        }
    }
}

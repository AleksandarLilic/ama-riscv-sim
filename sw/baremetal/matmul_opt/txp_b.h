#include <stdint.h>
#include "common.h"
#include "common_math.h"

#ifdef NF_INT8
#include "test_matrices_int8.h"
int8_t bt[B_COLS][A_COLS_B_ROWS]; // swapped dims because B is transposed

void transpose() {
    for (size_t j = 0; j < (B_COLS >> 2); j++) {
        for (size_t k = 0; k < (A_COLS_B_ROWS >> 2); k++) {

            int8x8_t bs_t16_02, bs_t16_13;
            _simd_txp_4x4_int8(B_COLS, b, k, j, &bs_t16_02, &bs_t16_13);

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

#endif

#ifdef NF_INT16
#include "test_matrices_int16.h"
int16_t bt[B_COLS][A_COLS_B_ROWS]; // swapped dims because B is transposed

void transpose() {
    for (size_t j = 0; j < (B_COLS >> 1); j++) {
        for (size_t k = 0; k < (A_COLS_B_ROWS >> 1); k++) {

            int16x4_t bs_t16;
            _simd_txp_2x2_int16(B_COLS, b, k, j, &bs_t16);

            v_store_int16x2(&bt[(j<<1) + 0][(k<<1)], bs_t16.w.lo);
            v_store_int16x2(&bt[(j<<1) + 1][(k<<1)], bs_t16.w.hi);
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
            #if A_COLS_B_ROWS >= 4
            int32_t c_acc = c[i][j];
            const size_t ks = 1 + 1; // step - 1 H to W load, + 1 for 2x at once
            for (size_t k = 0; k < (A_COLS_B_ROWS >> ks); k++) {
                // load first to prevent load-to-use dependency
                const int16x2_t as_0 = v_load_int16x2(&a[i][(k<<ks)]);
                const int16x2_t as_1 = v_load_int16x2(&a[i][(k<<ks)+2]);
                const int16x2_t bs_0 = v_load_int16x2(&bt[j][(k<<ks)]);
                const int16x2_t bs_1 = v_load_int16x2(&bt[j][(k<<ks)+2]);
                // dotp
                asm volatile (
                    "dot16 %[c], %[a0], %[b0]\n\t" // scheduling?
                    "dot16 %[c], %[a1], %[b1]\n\t"
                    : [c] "+r" (c_acc)
                    : [a0] "r" (as_0), [a1] "r" (as_1),
                      [b0] "r" (bs_0), [b1] "r" (bs_1)
                    :
                );
            }
            // store back
            c[i][j] = c_acc;

            #else // A_COLS_B_ROWS < 4
            c[i][j] = _simd_dot_product_int16(a[i], bt[j], A_COLS_B_ROWS);
            #endif
        }
    }
}

#endif

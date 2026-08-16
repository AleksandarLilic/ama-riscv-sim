#include <stdint.h>
#include "common.h"
#include "common_math.h"
#include "common_math_simd_v_load_store.h"

#ifdef NF_INT8
#include "test_matrices_int8.h"
int8_t b_T[N][K]; // swapped dims because B is transposed

void transpose() {
    // the whole blocked loop is the library driver now
    m_txp_i8(K, N, &b[0][0], N, &b_T[0][0], K);
    /*
    for (size_t k = 0; k < (K); k++) {
        printf("\n");
        for (size_t n = 0; n < (N); n++) {
            printf("%d, ", b_T[n][k]);
        }
    }
    printf("\n");
    */
}

void matmul() {
    for (size_t m = 0; m < M; m++) { // over A rows
        for (size_t n = 0; n < N; n++) { // over b.T rows
            #if K >= 8
            int32_t c_acc = c[m][n];
            const size_t ks = 2 + 1; // step: 2 B to W load, + 1 for 2x at once
            for (size_t k = 0; k < (K >> ks); k++) {
                // load first to prevent load-to-use dependency
                const int8x4_t as_0 = v_load_int8x4(&a[m][(k<<ks)]);
                const int8x4_t as_1 = v_load_int8x4(&a[m][(k<<ks)+4]);
                const int8x4_t bs_0 = v_load_int8x4(&b_T[n][(k<<ks)]);
                const int8x4_t bs_1 = v_load_int8x4(&b_T[n][(k<<ks)+4]);
                // dotp
                asm volatile (
                    "dot8 %[c], %[a1], %[b1]\n\t"
                    "dot8 %[c], %[a0], %[b0]\n\t" // scheduling
                    : [c] "+r" (c_acc)
                    : [a0] "r" (as_0), [a1] "r" (as_1),
                      [b0] "r" (bs_0), [b1] "r" (bs_1)
                    :
                );
            }
            // store back
            c[m][n] = c_acc;

            #else // K < 8
            c[m][n] = m_dotv_i8_i8(a[m], b_T[n], K);
            #endif
        }
    }
}

#endif

#ifdef NF_INT16
#include "test_matrices_int16.h"
int16_t b_T[N][K]; // swapped dims because B is transposed

void transpose() {
    m_txp_i16(K, N, &b[0][0], N, &b_T[0][0], K);
    /*
    for (size_t k = 0; k < (K); k++) {
        printf("\n");
        for (size_t n = 0; n < (N); n++) {
            printf("%d, ", b_T[n][k]);
        }
    }
    printf("\n");
    */
}

void matmul() {
    for (size_t m = 0; m < M; m++) { // over A rows
        for (size_t n = 0; n < N; n++) { // over b.T rows
            #if K >= 4
            int32_t c_acc = c[m][n];
            const size_t ks = 1 + 1; // step: 1 H to W load, + 1 for 2x at once
            for (size_t k = 0; k < (K >> ks); k++) {
                // load first to prevent load-to-use dependency
                const int16x2_t as_0 = v_load_int16x2(&a[m][(k<<ks)]);
                const int16x2_t as_1 = v_load_int16x2(&a[m][(k<<ks)+2]);
                const int16x2_t bs_0 = v_load_int16x2(&b_T[n][(k<<ks)]);
                const int16x2_t bs_1 = v_load_int16x2(&b_T[n][(k<<ks)+2]);
                // dotp
                asm volatile (
                    "dot16 %[c], %[a1], %[b1]\n\t"
                    "dot16 %[c], %[a0], %[b0]\n\t" // scheduling
                    : [c] "+r" (c_acc)
                    : [a0] "r" (as_0), [a1] "r" (as_1),
                      [b0] "r" (bs_0), [b1] "r" (bs_1)
                    :
                );
            }
            // store back
            c[m][n] = c_acc;

            #else // K < 4
            c[m][n] = m_dotv_i16_i16(a[m], b_T[n], K);
            #endif
        }
    }
}

#endif

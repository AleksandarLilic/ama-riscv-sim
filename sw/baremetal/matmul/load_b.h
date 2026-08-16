#include <stdint.h>
#include "common.h"
#include "common_math.h"
#include "common_math_simd_v_load_store.h"

#ifdef NF_INT8
#include "test_matrices_int8.h"

void matmul() {
    for (size_t n = 0; n < (N >> 2); n++) {
        for (size_t k = 0; k < (K >> 2); k++) {

            int8_t tmp[4][4] __attribute__((aligned(4)));
            m_txp_4x4_i8(&b[k<<2][n<<2], N, &tmp[0][0], 4);
            const int8x4_t bt0 = v_load_int8x4(&tmp[0][0]);
            const int8x4_t bt1 = v_load_int8x4(&tmp[1][0]);
            const int8x4_t bt2 = v_load_int8x4(&tmp[2][0]);
            const int8x4_t bt3 = v_load_int8x4(&tmp[3][0]);

            size_t cn = (n << 2);
            for (size_t m = 0; m < M; m++) {
                const int8x4_t as = v_load_int8x4(&a[m][k<<2]);

                // load first to prevent load-to-use dependency
                int32_t c_arr[4];
                c_arr[0] = c[m][cn + 0];
                c_arr[1] = c[m][cn + 1];
                c_arr[2] = c[m][cn + 2];
                c_arr[3] = c[m][cn + 3];

                // dotp
                asm volatile (
                    "dot8 %[c1], %[a], %[b1]\n\t"
                    "dot8 %[c2], %[a], %[b2]\n\t"
                    "dot8 %[c3], %[a], %[b3]\n\t"
                    "dot8 %[c0], %[a], %[b0]\n\t" // scheduling
                    : [c0] "+r" (c_arr[0]),
                      [c1] "+r" (c_arr[1]),
                      [c2] "+r" (c_arr[2]),
                      [c3] "+r" (c_arr[3])
                    : [a] "r" (as),
                      [b0] "r" (bt0),
                      [b1] "r" (bt1),
                      [b2] "r" (bt2),
                      [b3] "r" (bt3)
                    :
                );

                // store back
                c[m][cn + 0] = c_arr[0];
                c[m][cn + 1] = c_arr[1];
                c[m][cn + 2] = c_arr[2];
                c[m][cn + 3] = c_arr[3];
            }
        }
    }
}

#endif

#ifdef NF_INT16
#include "test_matrices_int16.h"

void matmul() {
    for (size_t n = 0; n < (N >> 1); n++) {
        for (size_t k = 0; k < (K >> 1); k++) {

            int16_t tmp[2][2] __attribute__((aligned(4)));
            m_txp_2x2_i16(&b[k<<1][n<<1], N, &tmp[0][0], 2);
            const int16x2_t bt0 = v_load_int16x2(&tmp[0][0]);
            const int16x2_t bt1 = v_load_int16x2(&tmp[1][0]);

            size_t cn = (n << 1);
            for (size_t m = 0; m < M; m++) {
                const int16x2_t as = v_load_int16x2(&a[m][k<< 1]);

                // load first to prevent load-to-use dependency
                int32_t c_arr[2];
                c_arr[0] = c[m][cn + 0];
                c_arr[1] = c[m][cn + 1];

                // dotp
                asm volatile (
                    "dot16 %[c1], %[a], %[b1]\n\t"
                    "dot16 %[c0], %[a], %[b0]\n\t" // scheduling
                    : [c0] "+r" (c_arr[0]),
                      [c1] "+r" (c_arr[1])
                    : [a] "r" (as),
                      [b0] "r" (bt0),
                      [b1] "r" (bt1)
                    :
                );

                // store back
                c[m][cn + 0] = c_arr[0];
                c[m][cn + 1] = c_arr[1];
            }
        }
    }
}

#endif

#include <stdint.h>
#include "common.h"
#include "common_math.h"

#ifdef NF_INT8
#include "test_matrices_int8.h"
#endif

#ifdef NF_INT16
#include "test_matrices_int16.h"
#endif

#if OPT_V_LOAD_B
#include "load_b.h"
#elif OPT_V_TILING
#include "tiling.h"
#elif OPT_V_TXP_B
#include "txp_b.h"
#endif

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
        #if OPT_V_TXP_B
        GLOBAL_SYMBOL("transpose_matrix_b");
        transpose();
        #endif

        GLOBAL_SYMBOL("compute");
        matmul();
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

        GLOBAL_SYMBOL("check");
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

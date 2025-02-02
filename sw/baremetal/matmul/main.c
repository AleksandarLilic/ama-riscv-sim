#include <stdint.h>
#include "common.h"

#include "test_matrices.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#define MAC c[i][j] += a[i][k] * b[k][j]

void set_c() {
    for (size_t i = 0; i < A_ROWS; i++) {
        for (size_t j = 0; j < B_COLS; j++) {
            c[i][j] = 0;
        }
    }
}

void main(void) {
    for (uint32_t i = 0; i < LOOPS; i++) {
        set_c();
        LOG_START;

        asm(".global compute");
        asm("compute:");
        #if LOOP_ORDER_IJK
        for (size_t i = 0; i < A_ROWS; i++) {
            for (size_t j = 0; j < B_COLS; j++) {
                for (size_t k = 0; k < A_COLS_B_ROWS; k++) MAC;
            }
        }
        #elif LOOP_ORDER_IKJ
        for (size_t i = 0; i < A_ROWS; i++) {
            for (size_t k = 0; k < A_COLS_B_ROWS; k++) {
                for (size_t j = 0; j < B_COLS; j++) MAC;
            }
        }
        #elif LOOP_ORDER_JIK
        for (size_t j = 0; j < B_COLS; j++) {
            for (size_t i = 0; i < A_ROWS; i++) {
                for (size_t k = 0; k < A_COLS_B_ROWS; k++) MAC;
            }
        }
        #elif LOOP_ORDER_JKI
        for (size_t j = 0; j < B_COLS; j++) {
            for (size_t k = 0; k < A_COLS_B_ROWS; k++) {
                for (size_t i = 0; i < A_ROWS; i++) MAC;
            }
        }
        #elif LOOP_ORDER_KIJ
        for (size_t k = 0; k < A_COLS_B_ROWS; k++) {
            for (size_t i = 0; i < A_ROWS; i++) {
                for (size_t j = 0; j < B_COLS; j++) MAC;
            }
        }
        #elif LOOP_ORDER_KJI
        for (size_t k = 0; k < A_COLS_B_ROWS; k++) {
            for (size_t j = 0; j < B_COLS; j++) {
                for (size_t i = 0; i < A_ROWS; i++) MAC;
            }
        }
        #endif
        LOG_STOP;

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

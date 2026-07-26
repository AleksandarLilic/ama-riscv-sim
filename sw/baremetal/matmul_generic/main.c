#include <stdint.h>
#include "common.h"

#include "test_matrices_int8.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#define MAC c[m][n] += a[m][k] * b[k][n]

void set_c() {
    for (size_t m = 0; m < M; m++) {
        for (size_t n = 0; n < N; n++) {
            c[m][n] = 0;
        }
    }
}

void main(void) {
    for (uint32_t i = 0; i < LOOPS; i++) {
        set_c();
        PROF_START;

        asm(".global compute");
        asm("compute:");
        #if LOOP_ORDER_MNK
        for (size_t m = 0; m < M; m++) {
            for (size_t n = 0; n < N; n++) {
                for (size_t k = 0; k < K; k++) MAC;
            }
        }
        #elif LOOP_ORDER_MKN
        for (size_t m = 0; m < M; m++) {
            for (size_t k = 0; k < K; k++) {
                for (size_t n = 0; n < N; n++) MAC;
            }
        }
        #elif LOOP_ORDER_NMK
        for (size_t n = 0; n < N; n++) {
            for (size_t m = 0; m < M; m++) {
                for (size_t k = 0; k < K; k++) MAC;
            }
        }
        #elif LOOP_ORDER_NKM
        for (size_t n = 0; n < N; n++) {
            for (size_t k = 0; k < K; k++) {
                for (size_t m = 0; m < M; m++) MAC;
            }
        }
        #elif LOOP_ORDER_KMN
        for (size_t k = 0; k < K; k++) {
            for (size_t m = 0; m < M; m++) {
                for (size_t n = 0; n < N; n++) MAC;
            }
        }
        #elif LOOP_ORDER_KNM
        for (size_t k = 0; k < K; k++) {
            for (size_t n = 0; n < N; n++) {
                for (size_t m = 0; m < M; m++) MAC;
            }
        }
        #endif
        PROF_STOP;

        /*
        printf("Result of matrix multiplication (C = A * B):\n");
        for (int m = 0; m < M; m++) {
            for (int n = 0; n < N; n++) {
                printf("%d ", c[m][n]);
            }
            printf("\n");
        }
        */

        asm(".global check");
        asm("check:");
        for (size_t m = 0; m < M; m++) {
            for (size_t n = 0; n < N; n++) {
                if (c[m][n] != ref[m][n]) {
                    write_mismatch(c[m][n], ref[m][n], m * N + n + 1);
                    fail();
                }
            }
        }
    }
    pass();
}

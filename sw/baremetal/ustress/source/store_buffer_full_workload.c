/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress the CPU store buffer.
 *
 * Theory:
 *   The program performs a large number of back-to-back writes to
 *   memory in order to saturate the CPU store buffer. The addresses
 *   written to by each store are in different cache lines to prevent
 *   them being merged in the store buffer. The code marks the writes
 *   as volatile to prevent the compiler from optimizing them away.
 */

#include <stdlib.h>
#include "cpuinfo.h"
#include "main.h"

#if USE_C

void stress(long runs) {
  volatile char* mem = malloc(L1D_CACHE_LINE_SIZE * STORE_BUFFER_SIZE * 2);
  for(long n=runs; n>0; n--) {
    for(int s=0; s<STORE_BUFFER_SIZE * 2; s++) {
      mem[L1D_CACHE_LINE_SIZE * s] = (char)n;
    }
  }
  free((void*)mem);
}

#else

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

__asm__ (
".equ L1D_CACHE_LINE_SIZE, " TOSTRING(L1D_CACHE_LINE_SIZE) "\n"
".equ STORE_BUFFER_SIZE, " TOSTRING(STORE_BUFFER_SIZE)     "\n"

"stress:                                                 \n"
"stp     fp, lr, [sp, #-16]!                             \n"
"mov     fp, sp                                          \n"
"stp     x19, xzr, [sp, #-16]!                           \n"
"mov     x19, x0                                         \n" // runs

"mov     w0, L1D_CACHE_LINE_SIZE * STORE_BUFFER_SIZE * 2 \n"
"bl      malloc                                          \n"

"0:                                                      \n"
"mov     w1, #0                                          \n" // s = 0
"mov     x2, x0                                          \n" // mem
"1:                                                      \n"
"strb    w19, [x2], L1D_CACHE_LINE_SIZE                  \n" // mem[L1D_CACHE_LINE_SIZE * s] = (char)n
"add     x1, x1, #1                                      \n"
"cmp     x1, STORE_BUFFER_SIZE * 2                       \n"
"blo     1b                                              \n"
"subs    x19, x19, #1                                    \n"
"bne     0b                                              \n"

"bl      free                                            \n"

"ldp     x19, xzr, [sp], #16                             \n"
"ldp     fp, lr, [sp], #16                               \n"
"ret                                                     \n"
);

#endif

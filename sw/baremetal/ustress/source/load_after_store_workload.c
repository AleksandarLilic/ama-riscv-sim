/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress load after store scheduling.
 *
 * Theory:
 *   The program performs a large number of back-to-back loads to
 *   the same address as a preceeding store in order to stress
 *   load after store speculation. The address that is accessed is
 *   repeatedly changed so as to thwart attempts to spot load after
 *   store instances and handle them more efficiently.
 */

#include <stdint.h>
#include "cpuinfo.h"
#include "main.h"

#if USE_C

#ifdef CPU_AMA_RISCV
static int32_t vars[64 * L1D_CACHE_LINE_SIZE];
#else
static int64_t vars[64 * L1D_CACHE_LINE_SIZE];
#endif

void stress(long runs) {
  for(int n=0; n<(64 * L1D_CACHE_LINE_SIZE);n++) {
    vars[n] = 1;
  }
  #ifdef CPU_AMA_RISCV
  volatile int32_t* var1 = vars;
  #else
  volatile int64_t* var1 = vars;
  #endif
  volatile int8_t* var2 = (int8_t*)vars;
  #define W(i) var1[(i * L1D_CACHE_LINE_SIZE) >> (var1[0] - 1)]
  #define R(i) var2[(i * L1D_CACHE_LINE_SIZE * 8) + 0] + var2[(i * L1D_CACHE_LINE_SIZE * 8) + 1] + var2[(i * L1D_CACHE_LINE_SIZE * 8) + 2] + var2[(i * L1D_CACHE_LINE_SIZE * 8) + 3] + var2[(i * L1D_CACHE_LINE_SIZE * 8) + 4]
  for(long n=runs; n>0; n--) {
    W(0) = R(32);
    W(1) = R(0);
    W(2) = R(1);
    W(3) = R(2);
    W(4) = R(3);
    W(5) = R(4);
    W(6) = R(5);
    W(7) = R(6);
    W(8) = R(7);
    W(9) = R(8);
    W(10) = R(9);
    W(11) = R(10);
    W(12) = R(11);
    W(13) = R(12);
    W(14) = R(13);
    W(15) = R(14);
    W(16) = R(15);
    W(17) = R(16);
    W(18) = R(17);
    W(19) = R(18);
    W(20) = R(19);
    W(21) = R(20);
    W(22) = R(21);
    W(23) = R(22);
    W(24) = R(23);
    W(25) = R(24);
    W(26) = R(25);
    W(27) = R(26);
    W(28) = R(27);
    W(29) = R(28);
    W(30) = R(29);
    W(31) = R(30);
    W(32) = R(31);
  }
}

#else

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

__asm__ (
".equ L1D_CACHE_LINE_SIZE, " TOSTRING(L1D_CACHE_LINE_SIZE) "\n"

".section .bss                              \n"
"vars: .fill 64 * L1D_CACHE_LINE_SIZE, 8, 0 \n"

".section .text                            \n"

".macro R i                                \n"
"mov     w3, \\i * L1D_CACHE_LINE_SIZE * 8 \n" // i * L1D_CACHE_LINE_SIZE * 8
"add     x3, x3, x1                        \n" // &var2[(i * L1D_CACHE_LINE_SIZE * 8) + 0]
"ldrb    w2, [x3]                          \n" // var2[(i * L1D_CACHE_LINE_SIZE * 8) + 0]
"ldrb    w4, [x3, #1]                      \n" // var2[(i * L1D_CACHE_LINE_SIZE * 8) + 1]
"add     w2, w2, w4                        \n"
"ldrb    w4, [x3, #2]                      \n" // var2[(i * L1D_CACHE_LINE_SIZE * 8) + 2]
"add     w2, w2, w4                        \n"
"ldrb    w4, [x3, #3]                      \n" // var2[(i * L1D_CACHE_LINE_SIZE * 8) + 3]
"add     w2, w2, w4                        \n"
"ldrb    w4, [x3, #4]                      \n" // var2[(i * L1D_CACHE_LINE_SIZE * 8) + 4]
"add     w2, w2, w4                        \n"
"and     w2, w2, #0xFF                     \n"
".endm                                     \n"

".macro W i                                \n"
"mov     w3, \\i * L1D_CACHE_LINE_SIZE     \n" // i * L1D_CACHE_LINE_SIZE
"ldr     x4, [x1]                          \n" // var1[0]
"sdiv    x3, x3, x4                        \n" // (i * L1D_CACHE_LINE_SIZE) / var1[0]
"str     x2, [x1, x3, LSL #3]              \n" // var1[(i * L1D_CACHE_LINE_SIZE) / var1[0]]
".endm                                     \n"

"stress:                                   \n"
"mov     w1, #1                            \n"
"mov     w2, #1                            \n"
"adr     x3, vars                          \n"
"mov     w4, 64 * L1D_CACHE_LINE_SIZE / 2  \n"
"0:                                        \n"
"stp     x1, x2, [x3], #16                 \n"
"subs    w4, w4, #1                        \n"
"bne     0b                                \n"

"adr     x1, vars   \n"
"1:                 \n"
"R 32               \n"
"W  0               \n"
"R  0               \n"
"W  1               \n"
"R  1               \n"
"W  2               \n"
"R  2               \n"
"W  3               \n"
"R  3               \n"
"W  4               \n"
"R  4               \n"
"W  5               \n"
"R  5               \n"
"W  6               \n"
"R  6               \n"
"W  7               \n"
"R  7               \n"
"W  8               \n"
"R  8               \n"
"W  9               \n"
"R  9               \n"
"W 10               \n"
"R 10               \n"
"W 11               \n"
"R 11               \n"
"W 12               \n"
"R 12               \n"
"W 13               \n"
"R 13               \n"
"W 14               \n"
"R 14               \n"
"W 15               \n"
"R 15               \n"
"W 16               \n"
"R 16               \n"
"W 17               \n"
"R 17               \n"
"W 18               \n"
"R 18               \n"
"W 19               \n"
"R 19               \n"
"W 20               \n"
"R 20               \n"
"W 21               \n"
"R 21               \n"
"W 22               \n"
"R 22               \n"
"W 23               \n"
"R 23               \n"
"W 24               \n"
"R 24               \n"
"W 25               \n"
"R 25               \n"
"W 26               \n"
"R 26               \n"
"W 27               \n"
"R 27               \n"
"W 28               \n"
"R 28               \n"
"W 29               \n"
"R 29               \n"
"W 30               \n"
"R 30               \n"
"W 31               \n"
"R 31               \n"
"W 32               \n"
"subs    x0, x0, #1 \n"
"bne     1b         \n"

"ret                \n"
);

#endif

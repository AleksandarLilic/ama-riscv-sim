/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress CPU with back-to-back 64-biit integer multiplies.
 *
 * Theory:
 *   The program performs back-to-back 64-bit integer multiplications where the
 *   result of one operation is needed for the next operation.
 */

#include <stdint.h>
#include "main.h"

#if USE_C

static int64_t kernel(long runs, int64_t result, int64_t mul) {
  for(long n=runs; n>0; n--) {
    result *= mul;
    result *= mul;
    result *= mul;
    result *= mul;
  }
  return result;
}

#else

int64_t kernel(long, int64_t, int64_t);

__asm__ (
"kernel:            \n"
"0:                 \n"
"mul     x1, x1, x2 \n" // result *= mul
"mul     x1, x1, x2 \n" // result *= mul
"mul     x1, x1, x2 \n" // result *= mul
"mul     x1, x1, x2 \n" // result *= mul
"subs    x0, x0, #1 \n" // n--
"bne     0b         \n"
"mov     x0, x1     \n"
"ret                \n"
);

#endif

void stress(long runs) {
  /* This volatile use of result should prevent the computation from being optimised away by the compiler. */
  int64_t result;
  volatile int64_t a = 99, b = 457;
  *((volatile int64_t*)&result) = kernel(runs, a, b);
}

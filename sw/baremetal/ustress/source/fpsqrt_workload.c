/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress CPU floating-point unit with square roots.
 *
 * Theory:
 *   The program peforms back-to-back double square roots where the
 *   result of one operation is needed for the next operation.
 */

#include <math.h>
#include <stdlib.h>
#include "main.h"

#if USE_C

static double kernel(long runs, double result) {
  for(long n=runs; n>0; n--) {
    result = sqrt(result);
    result = sqrt(result);
    result = sqrt(result);
    result = sqrt(result);
  }
  return result;
}

#else

double kernel(long, double);

__asm__ (
"kernel:            \n"
"0:                 \n"
"fsqrt   d0, d0     \n" // result = sqrt(result)
"fsqrt   d0, d0     \n" // result = sqrt(result)
"fsqrt   d0, d0     \n" // result = sqrt(result)
"fsqrt   d0, d0     \n" // result = sqrt(result)
"subs    x0, x0, #1 \n" // n--
"bne     0b         \n"
"ret                \n"
);

#endif

void stress(long runs) {
  /* This volatile use of result should prevent the computation from being optimised away by the compiler. */
  double result;
  *((volatile double*)&result) = kernel(runs, 1e20);
}

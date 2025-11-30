/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress CPU floating-point unit with multiplies.
 *
 * Theory:
 *   The program performs back-to-back double multiplications where the
 *   result of one operation is needed for the next operation.
 */

#include <stdlib.h>
#include "main.h"

#if USE_C

static double kernel(long runs, double result, double mul) {
  for(long n=runs; n>0; n--) {
    result *= mul;
    result *= mul;
    result *= mul;
    result *= mul;
  }
  return result;
}

#else

double kernel(long, double, double);

__asm__ (
"kernel:            \n"
"0:                 \n"
"fmul    d0, d0, d1 \n" // result *= mul
"fmul    d0, d0, d1 \n" // result *= mul
"fmul    d0, d0, d1 \n" // result *= mul
"fmul    d0, d0, d1 \n" // result *= mul
"subs    x0, x0, #1 \n" // n--
"bne     0b         \n"
"ret                \n"
);

#endif

void stress(long runs) {
  /* This volatile use of result should prevent the computation from being optimised away by the compiler. */
  double result;
  *((volatile double*)&result) = kernel(runs, 1e20, 2.1);
}

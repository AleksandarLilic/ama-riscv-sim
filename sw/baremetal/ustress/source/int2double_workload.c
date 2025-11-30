/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *  This program performs repeated int to double conversions.
 */

#include "main.h"

#if USE_C

void stress(long runs) {
  double result = 0;
  for(long n=runs; n>0; n--) {
    result += (double)n;
  }

  /* This volatile use of result should prevent the computation from being optimised away by the compiler. */
  *(volatile double*)&result = result;
}

#else

__asm__ (
"stress:            \n"
"movi    d0, #0     \n" // result = 0
"0:                 \n"
"scvtf   d1, x0     \n" // (double)n
"fadd    d0, d0, d1 \n" // result += (double)n
"subs    x0, x0, #1 \n" // n--
"bne     0b         \n"
"ret                \n"
);

#endif

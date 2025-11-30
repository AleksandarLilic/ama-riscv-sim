/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *  This program performs repeated double to int conversions.
 */

#include "main.h"

#if USE_C

void stress(long runs) {
  double d = 2.345;
  int result = 0;
  for(long n=runs; n>0; n--) {
    result += (int)d;
    d += 0.1;
  }

  /* This volatile use of result should prevent the computation from being optimised away by the compiler. */
  *(volatile int*)&result = result;
}

#else

__asm__ (
".balign 16                         \n"
"seed: .double 2.345                \n"
"step: .double 0.1                  \n"

"stress:                            \n"
"adr     x1, seed                   \n"
"ldp     d0, d1, [x1]               \n"

"mov     w1, #0                     \n" // result = 0

"0:                                 \n"
"fcvtzs  x2, d0                     \n" // (int)d
"add     x1, x1, x2                 \n" // result += (int)d
"fadd    d0, d0, d1                 \n" // d += 0.1
"subs    x0, x0, #1                 \n" // n--
"bne     0b                         \n"

"ret                                \n"
);

#endif

/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress CPU direct branch predicition.
 *
 * Theory:
 *   The program consists of a sequence of direct branches (the if statements). The
 *   direction of each branch is determined by a simple pseudo random number generator
 *   that should be sufficient to defeat most branch predictors. The random number is
 *   computed immediately prior to each branch and each ramdon number depends on the
 *   previous one; these techniques should reduce the possibility for the processor
 *   to compute the branch direction well in advance of each branch.
 *
 *   It is advisable to check the disassembly to ensure that the compiler has indeed
 *   generated a conditional direct branch for each if statement and has not optimised
 *   these in some other way.
 */

#include <stdint.h>
#include <stdlib.h>
#include "main.h"

#if USE_C

void stress(long runs) {
  uint16_t lfsr = 0xACE1u;
  int result = 0;
  for(long n=runs; n>0; n--) {
    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;

    lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
    if (lfsr & 1) result = (result << 1) ^ 11;
  }

  /* This volatile use of result should prevent the above code from being optimised away by the compiler. */
  *((volatile int*)&result) = result + 1;
}

#else

__asm__ (
"stress:                     \n"
"mov     w1, #0xACE1         \n" // lfsr
"mov     w2, #0              \n" // result
"mov     w3, #11             \n"
"0:                          \n"
".rept 16                    \n"
"lsr     w4, w1, #1          \n" // lfsr >> 1
"eor     w1, w1, w1, LSR #2  \n" // lfsr >> 0 ^ lfsr >> 2
"eor     w1, w1, w1, LSR #3  \n" // lfsr >> 0 ^ lfsr >> 2 ^ lfsr >> 3 ^ lfsr >> 5
"and     w1, w1, #1          \n" // (lfsr >> 0 ^ lfsr >> 2 ^ lfsr >> 3 ^ lfsr >> 5) & 1
"orr     w1, w4, w1, LSL #15 \n" // lfsr >> 1 | ((lfsr >> 0 ^ lfsr >> 2 ^ lfsr >> 3 ^ lfsr >> 5) & 1) << 15
"tbz     w1, #0, 1f          \n"
"eor     w2, w3, w2, LSL #1  \n" // (result << 1) ^ 11
"1:                          \n"
".endr                       \n"
"subs    x0, x0, #1          \n"
"bne     0b                  \n"
"ret                         \n"
);

#endif

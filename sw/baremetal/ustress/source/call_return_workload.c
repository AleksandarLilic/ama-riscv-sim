/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress CPU call return prediction.
 *
 * Theory:
 *   The program creates a long chain of calls to randomly selected functions.
 *   Each function uses the return value from the function it calls in a
 *   further computation so that the calls are not optimised away by the
 *   compiler. The call stack thus ends up with a long sequence of return
 *   addresses that are to the CPU non-predictable.
 */

#include <stdint.h>
#include <stdlib.h>
#include "main.h"

#if USE_C

static uint16_t fA(int n, uint16_t p);
static uint16_t fB(int n, uint16_t p);
static uint16_t fC(int n, uint16_t p);
static uint16_t fD(int n, uint16_t p);
static uint16_t fE(int n, uint16_t p);
static uint16_t fF(int n, uint16_t p);
static uint16_t fG(int n, uint16_t p);
static uint16_t fH(int n, uint16_t p);

static uint16_t (*calls[])(int n, uint16_t p) = {
  &fA, &fB, &fC, &fD, &fE, &fF, &fG, &fH
};

static uint16_t fA(int n, uint16_t lfsr) {
  n--;
  if (n == 0) return lfsr;
  lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
  return calls[lfsr & 7](n, lfsr) ^ 128;
}

static uint16_t fB(int n, uint16_t lfsr) {
  n--;
  if (n == 0) return lfsr;
  lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
  return calls[lfsr & 7](n, lfsr) ^ 64;
}

static uint16_t fC(int n, uint16_t lfsr) {
  n--;
  if (n == 0) return lfsr;
  lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
  return calls[lfsr & 7](n, lfsr) ^ 32;
}

static uint16_t fD(int n, uint16_t lfsr) {
  n--;
  if (n == 0) return lfsr;
  lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
  return calls[lfsr & 7](n, lfsr) ^ 16;
}

static uint16_t fE(int n, uint16_t lfsr) {
  n--;
  if (n == 0) return lfsr;
  lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
  return calls[lfsr & 7](n, lfsr) ^ 8;
}

static uint16_t fF(int n, uint16_t lfsr) {
  n--;
  if (n == 0) return lfsr;
  lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
  return calls[lfsr & 7](n, lfsr) ^ 4;
}

static uint16_t fG(int n, uint16_t lfsr) {
  n--;
  if (n == 0) return lfsr;
  lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
  return calls[lfsr & 7](n, lfsr) ^ 2;
}

static uint16_t fH(int n, uint16_t lfsr) {
  n--;
  if (n == 0) return lfsr;
  lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
  return calls[lfsr & 7](n, lfsr) ^ 1;
}

void stress(long runs) {
  uint16_t r = 0xACE1u;
  for(int n = runs; n>0; n--) {
    r = fA(10000, r);
  }

  /* This volatile use of result should prevent the above code from being optimised away by the compiler. */
  *((volatile uint16_t*)&r) = r + 1;
}

#else

__asm__ (
".section .bss                     \n"
".balign 64                        \n"
"calls: .fill 8, 8, 0              \n"

".section .text                    \n"
".macro fN letter number           \n"
".balign 64                        \n"
"f\\letter:                        \n"
"subs    x0, x0, #1                \n" // n--
"bne     0f                        \n"
"mov     w0, w1                    \n"
"ret                               \n" // return lfsr
"0:                                \n"
"stp     xzr, lr, [sp, #-16]!      \n"
"lsr     w2, w1, #1                \n"
"eor     w1, w1, w1, LSR #2        \n"
"eor     w1, w1, w1, LSR #3        \n"
"and     w1, w1, #1                \n"
"orr     w1, w2, w1, LSL #15       \n" // lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15)
"and     w2, w1, #7                \n" // lfsr & 7
"ldr     x2, [x3, x2, LSL #3]      \n"
"blr     x2                        \n" // calls[lfsr & 7](n, lfsr)
"ldp     xzr, lr, [sp], #16        \n"
"eor     w0, w0, \\number          \n" // calls[lfsr & 7](n, lfsr) ^ N
"ret                               \n" // return calls[lfsr & 7](n, lfsr) ^ N
".endm                             \n"

"fN A,128                          \n"
"fN B,64                           \n"
"fN C,32                           \n"
"fN D,16                           \n"
"fN E,8                            \n"
"fN F,4                            \n"
"fN G,2                            \n"
"fN H,1                            \n"

"stress:                           \n"
"adr     x3, calls + 64            \n"
"adr     x1, fG                    \n"
"adr     x2, fH                    \n"
"stp     x1, x2, [x3, #-16]!       \n"
"adr     x1, fE                    \n"
"adr     x2, fF                    \n"
"stp     x1, x2, [x3, #-16]!       \n"
"adr     x1, fC                    \n"
"adr     x2, fD                    \n"
"stp     x1, x2, [x3, #-16]!       \n"
"adr     x1, fA                    \n"
"adr     x2, fB                    \n"
"stp     x1, x2, [x3, #-16]!       \n"
"stp     x19, lr, [sp, #-16]!      \n"
"mov     w1, #0xACE1               \n" // r = 0xACE1u
"mov     x19, x0                   \n" // runs
"0:                                \n"
"mov     w0, #10000                \n"
"bl      fA                        \n" // fA(10000, r)
"mov     w1, w0                    \n" // r = fA(10000, r)
"subs    x19, x19, #1              \n" // n--
"bne     0b                        \n"
"ldp     x19, lr, [sp], #16        \n"
"ret                               \n"
);

#endif

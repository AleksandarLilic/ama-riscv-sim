/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress CPU indirect branch prediction.
 *
 * Theory:
 *   The program uses makes a large number of indirect branches using a
 *   random index into an array of code labels. The branch to each label
 *   is achieved using a computed goto (gcc specific feature). To
 *   prevent the compiler from optimising all the labels down to a single
 *   address then the code at each label performs a read from a volatile
 *   variable (and the resulting sequence of reads for each label is hard
 *   to optimise away). The compution to determin the next destination
 *   label is performed as close as possible to the branch to reduce the
 *   opportunity for the CPU to compute the address well in advance.
 *
 *   It is advisabke to check the disassembly to ensure that the generated
 *   code does contain distinct addresses for each label, and that the
 *   address computation occurs immediately before the branch.
 */

#include <stdint.h>
#include <stdlib.h>
#include "cpuinfo.h"

#ifdef CPU_AMA_RISCV
#include "common.h"
#endif

#if USE_C

static volatile int v = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
int main(void) {
  void* labels[] = {
    &&l0, &&l1, &&l2, &&l3, &&l4, &&l5, &&l6, &&l7,
    &&l8, &&l9, &&l10, &&l11, &&l12, &&l13, &&l14, &&l15,
    &&l16, &&l17, &&l18, &&l19, &&l20, &&l21, &&l22, &&l23,
    &&l24, &&l25, &&l26, &&l27, &&l28, &&l29, &&l30, &&l31
  };
  for(unsigned int mask = 0x1F; mask > 0; mask >>= 1) {
    uint16_t lfsr = 0xACE1u;
    #ifdef CPU_AMA_RISCV
    long n=100;
    #else
    long n=3000000;
    #endif
    while(n > 0) {
      l31:  v;
      l30:  v;
      l29:  v;
      l28:  v;
      l27:  v;
      l26:  v;
      l25:  v;
      l24:  v;
      l23:  v;
      l22:  v;
      l21:  v;
      l20:  v;
      l19:  v;
      l18:  v;
      l17:  v;
      l16:  v;
      l15:  v;
      l14:  v;
      l13:  v;
      l12:  v;
      l11:  v;
      l10:  v;
      l9:   v;
      l8:   v;
      l7:   v;
      l6:   v;
      l5:   v;
      l4:   v;
      l3:   v;
      l2:   v;
      l1:   v;
      lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15);
      goto *labels[lfsr & mask];
      l0:   n--;
    }

    /* This volatile use of result should prevent the above code from being optimised away by the compiler. */
    *((volatile uint16_t*)&lfsr) = lfsr + 1;
  }

  #ifdef CPU_AMA_RISCV
  pass();
  #endif

  return EXIT_SUCCESS;
}
#pragma GCC diagnostic pop

#else

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

__asm__ (
".equ L1D_CACHE_LINE_SIZE, " TOSTRING(L1D_CACHE_LINE_SIZE) "\n"

".globl main                          \n"
"main:                                \n"

"# Save frame pointer                 \n"
"stp     fp, lr, [sp, #-16]!          \n"
"mov     fp, sp                       \n"

"# Align stack to cache line          \n"
"mov     x0, sp                       \n"
"and     sp, x0, -L1D_CACHE_LINE_SIZE \n"

"# Fill labels[]                      \n"
"mov     w0, #15                      \n"
"adr     x1, 2f + 4                   \n"
"adr     x2, 2f                       \n"
"0:                                   \n"
"stp     x1, x2, [sp, #-16]!          \n"
"add     x1, x1, #8                   \n"
"add     x2, x2, #8                   \n"
"subs    w0, w0, #1                   \n"
"bne     0b                           \n"
"adr     x1, 3f                       \n"
"stp     x1, x2, [sp, #-16]!          \n"

"# Mask = 0x1F                        \n"
"mov     w0, #0x1F                    \n"

"# Outer loop                         \n"
"1:                                   \n"

"# LFSR = 0xACE1                      \n"
"mov     w1, #0xACE1                  \n"

"# N = 3000000                        \n"
"movz    w2, 3000000 & 0xFFFF         \n"
"movk    w2, 3000000 >> 16, LSL #16   \n"

"# Inner loop                         \n"
"/* l31: v; *                         \n"
" * l30: v; *                         \n"
" * l29: v; *                         \n"
" * ...     */                        \n"
"2:                                   \n"
".rept 31                             \n"
"nop                                  \n"
".endr                                \n"

"# lfsr = (lfsr >> 1) | ((((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1) << 15); \n"
"lsr     w3, w1, #1                   \n"
"eor     w1, w1, w1, LSR #2           \n"
"eor     w1, w1, w1, LSR #3           \n"
"and     w1, w1, #1                   \n"
"orr     w1, w3, w1, LSL #15          \n"

"# goto *labels[lfsr & mask];         \n"
"and     w3, w0, w1                   \n"
"ldr     x3, [sp, x3, LSL #3]         \n"
"br      x3                           \n"

"# l0: n--;                           \n"
"3:                                   \n"
"subs    x2, x2, #1                   \n"
"bne     2b                           \n"

"# mask >>= 1                         \n"
"adds    w0, wzr, w0, lsr #1          \n"

"# mask > 0                           \n"
"bne     1b                           \n"

"# Restore stack pointer              \n"
"mov     sp, fp                       \n"
"# Restore frame pointer              \n"
"ldp     fp, lr, [sp], #16            \n"

"# return EXIT_SUCCESS;               \n"
"ret                                  \n"
);

#endif

/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress L1D cache with misses.
 *
 * Theory:
 *   The program iterates through a block of memory whose size is twice
 *   that of the cache, by necessity forcing cache refills to occur. The
 *   accesses are actually targetted at the first cache index only, since
 *   exceeding the associativity at a single index is sufficient to
 *   generate refills. The access pattern is non-linear to twart trivial
 *   prefetching attempts.
 */

#include <stdlib.h>
#include "cpuinfo.h"
#include "main.h"

void** setup(void) {
  /* Create and initialise a block of memory with a non-linear pointer chain. */
  size_t memSize = L1D_CACHE_SIZE * 2;
  void** mem = malloc(memSize);
  int stepSize = (L1D_CACHE_LINE_SIZE * L1D_CACHE_ASSOCIATIVITY) / sizeof(void*);
  int maxSet = memSize / (stepSize * sizeof(void*));
  int idx = 0;
  for(int set = 1; set < maxSet; set++) {
    int idxNext = (set & 1) ? (maxSet - set) : set;
    mem[stepSize * idx] = &mem[stepSize * idxNext];
    idx = idxNext;
  }
  mem[stepSize * idx] = NULL;
  return mem;
}

#if USE_C

void stress(long runs) {
  void** mem = setup();

  /* Repeatedly follow the pointer chain to generate cache refills. */
  int sum = 0;
  for(long n=runs; n>0; n--) {
    void** next = mem;
    do {
      sum++;
      next = (void**)*next;
    } while(next != NULL);
  }

  /* This volatile use of result should prevent the computation from being optimised away by the compiler. */
  *((volatile int*)&sum) = sum;

  free((void*)mem);
}

#else

__asm__ (
"stress:                       \n"
"stp     fp, lr, [sp, #-16]!   \n"
"mov     fp, sp                \n"
"stp     x19, xzr, [sp, #-16]! \n"
"mov     x19, x0               \n" // runs

"bl      setup                 \n"

"0:                            \n"
"mov     x1, x0                \n" // next = mem
"1:                            \n"
"ldr     x1, [x1]              \n" // next = *next
"cbnz    x1, 1b                \n"
"subs    x19, x19, #1          \n"
"bne     0b                    \n"

"bl      free                  \n"

"ldp     x19, xzr, [sp], #16   \n"
"ldp     fp, lr, [sp], #16     \n"
"ret                           \n"
);

#endif

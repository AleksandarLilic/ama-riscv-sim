/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress the CPU DTLB0 with misses.
 *
 * Theory:
 *   The program iterates through memory with a stride equal to the
 *   page size. This results in repeated TLB accesses from different
 *   pages to overload the L1 DTLB entries.
 */

#include <assert.h>
#include <stdlib.h>
#include "cpuinfo.h"
#include "main.h"

#ifdef _WIN32
#define aligned_alloc(a, b) _aligned_malloc(b, a)
#define free  _aligned_free
#endif

volatile char* setup(void) {
  // #pages = #TLB-entries * 4
  int maxSize = L1D_TLB_SIZE * 4;
  // guard buffer overflow
  int guardSize = maxSize + 10;
  assert((PAGE_SIZE + L1D_CACHE_LINE_SIZE) * (maxSize - 1) < (PAGE_SIZE * guardSize));

  volatile char* mem = aligned_alloc(PAGE_SIZE, PAGE_SIZE * guardSize);
  memset((void*)mem, 0, PAGE_SIZE * guardSize);
  return mem;
}

#if USE_C

void stress(long runs) {
  // #pages = #TLB-entries * 4
  int maxSize = L1D_TLB_SIZE * 4;
  volatile char* mem = setup();
  char sum = 0;
  for(long n=runs; n>0; n--) {
    for(int set=0; set<maxSize; set++) {
      // offset by l1d cache line size to not triggering l1d cache miss
      sum += mem[(PAGE_SIZE * set) + (L1D_CACHE_LINE_SIZE * set)];
      // introduce load-load dependency, mem is not changed as sum is always 0
      mem += sum;
    }
  }
  mem[0] = sum;  // store sum to volatile address so that it is not optimized away
  free((void*)mem);
}

#else

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

__asm__ (
".equ PAGE_SIZE, " TOSTRING(PAGE_SIZE)                     "\n"
".equ L1D_TLB_SIZE, " TOSTRING(L1D_TLB_SIZE)               "\n"
".equ L1D_CACHE_LINE_SIZE, " TOSTRING(L1D_CACHE_LINE_SIZE) "\n"

"stress:                             \n"
"stp     fp, lr, [sp, #-16]!         \n"
"mov     fp, sp                      \n"
"stp     x19, xzr, [sp, #-16]!       \n"
"mov     x19, x0                     \n" // runs

"bl      setup                       \n"

"0:                                  \n"
"mov     w1, #0                      \n" // index
"mov     w2, L1D_TLB_SIZE * 4        \n" // maxSize
"1:                                  \n"
"ldrb    w3, [x0, x1]                \n" // w3 = mem[(PAGE_SIZE * set) + (L1D_CACHE_LINE_SIZE * set)]
"add     w1, w1, PAGE_SIZE           \n"
"add     w1, w1, L1D_CACHE_LINE_SIZE \n"
"subs    w2, w2, #1                  \n"
"bne     1b                          \n"
"subs    x19, x19, #1                \n"
"bne     0b                          \n"

#ifdef _WIN32
"bl      _aligned_free               \n"
#else
"bl      free                        \n"
#endif

"ldp     x19, xzr, [sp], #16         \n"
"ldp     fp, lr, [sp], #16           \n"
"ret                                 \n"
);

#endif

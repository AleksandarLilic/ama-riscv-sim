/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program stresses the load-store pipeline with a memcpy entirely within L1D cache.
 *
 * Theory:
 *   The built-in mem copy is usally designed to be as efficient as possible, and may
 *   trigger the CPU prefetching hardware. This should fill up the load-store pipeline
 *   entirely with no dependency on other pipelines.
 */

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "cpuinfo.h"
#include "main.h"

#if USE_C

#ifdef _WIN32
#define aligned_alloc(a, b) _aligned_malloc(b, a)
#define free  _aligned_free
#endif

void stress(long runs) {
  char* mem = aligned_alloc(L1D_CACHE_SIZE, L1D_CACHE_SIZE);
  // Make sure the physical pages are allocated.
  // Reading from demand paging won't allocate physical memory. Instead, it
  // simply reads from a zerod page preallocated by kernel.
  memset(mem, 1, L1D_CACHE_SIZE);
  for(long n=runs; n>0; n--) {
    char* memA = mem;
    char* memB = mem + (L1D_CACHE_SIZE  / 2);
    memcpy(memB, memA, L1D_CACHE_SIZE / 2);
    // gcc 10.3 optimize the memcpy away without below line
    *(volatile char*)memB;
  }
  free(mem);
}

#else

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

__asm__ (
".equ L1D_CACHE_SIZE, " TOSTRING(L1D_CACHE_SIZE) "\n"

"stress:                            \n"
"stp     fp, lr, [sp, #-16]!        \n"
"mov     fp, sp                     \n"
"stp     x19, xzr, [sp, #-16]!      \n"

"mov     x19, x0                    \n" // runs

#ifdef _WIN32
"mov     w0, L1D_CACHE_SIZE         \n" // size
"mov     w1, L1D_CACHE_SIZE         \n" // alignment
"bl      _aligned_malloc            \n" // ON WINDOWS ORDER OF ARGUMENTS IS SWAPPED
#else
"mov     w0, L1D_CACHE_SIZE         \n" // alignment
"mov     w1, L1D_CACHE_SIZE         \n" // size
"bl      aligned_alloc              \n"
#endif

"mov     x1, #0x0101010101010101    \n" // Uses "MOV (bitmask immediate)" instruction
"mov     x2, #0x0101010101010101    \n" // Uses "MOV (bitmask immediate)" instruction
"mov     x3, x0                     \n" // mem
"mov     w4, L1D_CACHE_SIZE / 16    \n"
"0:                                 \n" // inlined memset loop
"stp     x1, x2, [x3], #16          \n" // memset(mem, 1, L1D_CACHE_SIZE)
"subs    w4, w4, #1                 \n"
"bne     0b                         \n"

"add     x1, x0, L1D_CACHE_SIZE / 2 \n"
"1:                                 \n"
"mov     w2, #0                     \n"
"2:                                 \n" // inlined memcpy loop
"ldr     x3, [x0, x2, LSL #3]       \n" // memcpy(memB, memA, L1D_CACHE_SIZE / 2)
"str     x3, [x1, x2, LSL #3]       \n" // memcpy(memB, memA, L1D_CACHE_SIZE / 2)
"add     w2, w2, #1                 \n"
"cmp     w2, L1D_CACHE_SIZE / 16    \n"
"bne     2b                         \n"
"subs    x19, x19, #1               \n"
"bne     1b                         \n"

#ifdef _WIN32
"bl      _aligned_free              \n"
#else
"bl      free                       \n"
#endif

"ldp     x19, xzr, [sp], #16        \n"
"ldp     fp, lr, [sp], #16          \n"
"ret                                \n"
);

#endif

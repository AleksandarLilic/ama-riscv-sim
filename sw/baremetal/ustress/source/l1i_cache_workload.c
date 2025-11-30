/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress CPU L1I cache with misses.
 *
 * Theory:
 *   The program makes repeated calls to functions that are aligned to
 *   page boundaries. Given sufficient different function calls then
 *   the calls exceed the associativity of the L1I cache and cause
 *   misses.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "cpuinfo.h"
#include "main.h"

#if USE_C

#define UNUSED(x) (void)x

#define FUNC(f) \
  static __attribute__((aligned(L1I_CACHE_SIZE))) void f(void* fp) { \
    void (**funcPtr)(void*) = fp; \
    void(*func)(void*) = *(funcPtr++); \
    (*func)(funcPtr); \
  }

FUNC(fA)
FUNC(fB)
FUNC(fC)
FUNC(fD)
FUNC(fE)
FUNC(fF)
FUNC(fG)
FUNC(fH)
FUNC(fI)
FUNC(fJ)
FUNC(fK)
FUNC(fL)

static __attribute__((aligned(L1I_CACHE_SIZE))) void fZ(void* fp) {
    UNUSED(fp);
}

static void (*funcs[])(void*) = {
  &fA, &fB, &fC, &fD, &fE, &fF, &fG, &fH, &fI, &fJ, &fK, &fL,
  &fZ
};

static void assertFuncsArePageAligned(void) {
  void(**funcPtr)(void*) = funcs;
  uintptr_t pageMask = (uintptr_t)(L1I_CACHE_SIZE - 1);
  while(*funcPtr) {
    uintptr_t diff = (uintptr_t)(*funcPtr) & pageMask;
    UNUSED(diff);
    assert(diff == 0);
    funcPtr++;
  }
}

static void assertFuncsAreDistinct(void) {
  void(**funcPtr1)(void*) = funcs;
  while(*funcPtr1) {
    void(**funcPtr2)(void*) = funcPtr1 + 1;
    while(*funcPtr2) {
      assert(*funcPtr1 != *funcPtr2);
      funcPtr2++;
    }
    funcPtr1++;
  }
}

void stress(long runs) {
  assertFuncsArePageAligned();
  assertFuncsAreDistinct();
  for(volatile long n=runs; n>0; n--) {
    (*funcs[0])((void*)funcs);
  }
}

#else

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

__asm__ (
".equ L1I_CACHE_SIZE, " TOSTRING(L1I_CACHE_SIZE) "\n"

".balign 128                        \n"
"funcs:                             \n"
".quad fA                           \n"
".quad fB                           \n"
".quad fC                           \n"
".quad fD                           \n"
".quad fE                           \n"
".quad fF                           \n"
".quad fG                           \n"
".quad fH                           \n"
".quad fI                           \n"
".quad fJ                           \n"
".quad fK                           \n"
".quad fL                           \n"
".quad fZ                           \n"

".macro fN letter                   \n"
".balign L1I_CACHE_SIZE             \n"
"f\\letter:                         \n"
"ldr     x1, [x0], #8               \n" // func = *(funcPtr++)
"br      x1                         \n" // (*func)(funcPtr)
".endm                              \n"

"fN A                               \n"
"fN B                               \n"
"fN C                               \n"
"fN D                               \n"
"fN E                               \n"
"fN F                               \n"
"fN G                               \n"
"fN H                               \n"
"fN I                               \n"
"fN J                               \n"
"fN K                               \n"
"fN L                               \n"
".balign L1I_CACHE_SIZE             \n"
"fZ:                                \n"
"ret                                \n"

"stress:                            \n"
"stp     fp, lr, [sp, #-16]!        \n"
"mov     fp, sp                     \n"
"stp     x19, x20, [sp, #-16]!      \n"
"ldr     x19, funcs                 \n"
"mov     x20, x0                    \n" // runs
"0:                                 \n"
"adr     x0, funcs                  \n" // (void*)funcs
"blr     x19                        \n" // (*funcs[0])((void*)funcs)
"subs    x20, x20, #1               \n" // n--
"bne     0b                         \n"
"ldp     x19, x20, [sp], #16        \n"
"ldp     fp, lr, [sp], #16          \n"
"ret                                \n"
);

#endif

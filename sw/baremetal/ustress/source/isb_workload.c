/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2022-2025 Arm Limited
 */

/*
 * Purpose:
 *   This program aims to stress the CPU by repeatedly flushing the
 *   pipeline with ISB instructions.
 *
 * Theory:
 *   The program performs a large number of back-to-back ISB
 *   instructions.
 */

#include <stdlib.h>
#include "cpuinfo.h"
#include "main.h"

void stress(long runs) {
  for(long n=runs; n>0; n--) {
    for(int m=10000; m>0; m--) {
#if defined(__aarch64__)
      __asm("isb\n");
      __asm("isb\n");
      __asm("isb\n");
      __asm("isb\n");
#elif defined(__x86_64__)
      __asm volatile ("mfence" ::: "memory");
      __asm volatile ("mfence" ::: "memory");
      __asm volatile ("mfence" ::: "memory");
      __asm volatile ("mfence" ::: "memory");
#else
#error Uknown architecture!
#endif
    }
  }
}

#include <stdint.h>
#include "common.h"
#include <string.h>

// align to cache line size
#define A_ALIGN __attribute__((aligned(CACHE_LINE_SIZE)))
#define C_ALIGN __attribute__((aligned(CACHE_LINE_SIZE)))

#if defined(LEN_TINY)
#include "test_arrays_tiny.h"
#elif defined(LEN_SMALL)
#include "test_arrays_small.h"
#elif defined(LEN_MEDIUM)
#include "test_arrays_medium.h"
#elif defined(LEN_LARGE)
#include "test_arrays_large.h"
#else
_Static_assert(0, "No array length defined or unsupported length specified");
#endif

#ifndef LOOPS
#define LOOPS 1
#endif

void main() {
    NF_IN c[ARR_LEN] C_ALIGN;
    #ifdef MEASURE_TIME
    uint32_t start_time = get_cpu_time();
    #endif
    for (uint32_t i = 0; i < LOOPS; i++) {
        PROF_START;
        memcpy(c, a, sizeof(NF_IN) * ARR_LEN);
        PROF_STOP;
    }
    #ifdef MEASURE_TIME
    uint32_t end_time = get_cpu_time();
    uint32_t time_diff = (end_time - start_time);
    printf("Time taken: %d ms\n", time_diff / 1000);
    #endif
    #ifdef CHECK_RESULTS
    for (size_t j = 0; j < ARR_LEN; j++) {
        if (c[j] != a[j]) {
            write_mismatch(c[j], a[j], j + 1); // +1 to avoid writing 0
        }
    }
    #endif
    pass();
}

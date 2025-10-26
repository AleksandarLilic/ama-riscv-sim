#include "common.h"

#ifndef LOOPS
#define LOOPS 1
#endif

// perfect LRU assumed
#define CACHE_LINE_SIZE 64
#define DCACHE_SETS 8
#define DCACHE_WAYS 2
#define ARR_LEN (CACHE_LINE_SIZE >> 3) * (DCACHE_SETS << 1)

#ifdef CACHE_ALIGNED
#define ALIGN __attribute__((aligned(CACHE_LINE_SIZE)))
#else
#define ALIGN
#endif

volatile int32_t data[DCACHE_WAYS+1][ARR_LEN] ALIGN = {0};

int32_t loads() {
    int32_t sink = 0;
    for (int i = 0; i < ARR_LEN; ++i) {
        for (int j = 0; j < (DCACHE_WAYS+1); ++j) {
            sink += data[j][i];
        }
    }
}

void main() {
    for (uint32_t i = 0; i < LOOPS; i++) {
        PROF_START;
        volatile int32_t res = loads();
        PROF_STOP;
    }
    pass();
}

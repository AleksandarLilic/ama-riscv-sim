#include "common.h"

#ifndef LOOPS
#define LOOPS 1
#endif

// perfect LRU assumed
#ifdef CACHE_ALIGNED
#define ALIGN __attribute__((aligned(CACHE_LINE_SIZE)))
#else
#define ALIGN
#endif

#define WORDS_PER_LINE (CACHE_LINE_SIZE >> 2)
#define ARR_LEN (DCACHE_SETS * WORDS_PER_LINE)

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

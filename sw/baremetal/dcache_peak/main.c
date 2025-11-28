#include "common.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#ifdef CACHE_ALIGNED
#define ALIGN __attribute__((aligned(CACHE_LINE_SIZE)))
#else
#define ALIGN
#endif

#define CACHE_LINE_SIZE_WORDS (CACHE_LINE_SIZE / sizeof(int32_t))

int32_t data[CACHE_LINE_SIZE_WORDS] ALIGN = {0};
int32_t source = 0xC0FFEE;
int32_t sink = 0xBADC0DE;

/*         : [dst] "=r" (sink[i]) \ */
#define ASM_BLOCK_LOAD \
    asm volatile ( \
        "lw %[r], 0(%[addr]);" \
        : [r] "=r" (sink_reg) \
        : [addr] "r" (&data[j]) \
        : \
    );

#define ASM_BLOCK_STORE \
    asm volatile ( \
        "sw %[r], 0(%[addr]);" \
        : \
        : [r] "r" (source_reg), \
          [addr] "r" (&data[j]) \
        : \
    );

#if defined(LOAD)
#define ASM_BLOCK \
    ASM_BLOCK_LOAD \
    ASM_BLOCK_LOAD
#elif defined(STORE)
#define ASM_BLOCK \
    ASM_BLOCK_STORE \
    ASM_BLOCK_STORE
#elif defined(MIXED)
#define ASM_BLOCK \
    ASM_BLOCK_LOAD; \
    ASM_BLOCK_STORE;
#else
_Static_assert(0, "Unsupported operation");
#endif

#define ASM_BLOCK_8 \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \
    ASM_BLOCK; \

void main() {
    PROF_START;
    register uint32_t sink_reg = sink;
    register uint32_t source_reg = source;
    for (uint32_t i = 0; i < LOOPS; i++) {
        #pragma GCC unroll 16
        for (uint32_t j = 0; j < CACHE_LINE_SIZE_WORDS; j++) {
            //sink[i] = data[i];
            ASM_BLOCK;
            ASM_BLOCK;
            ASM_BLOCK;
        }
    }
    PROF_STOP;

    //GLOBAL_SYMBOL(check);
    // just so it's not optimized away
    //for (int i = 0; i < CACHE_LINE_SIZE_WORDS; i++) {
    //    if (sink[i] != 0) {
    //        write_mismatch(sink[i], 0, 1);
    //        fail();
    //    }
    //}
    pass();
}

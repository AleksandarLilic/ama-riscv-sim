#include <stdint.h>
#include "common.h"
#include "common_math.h"
#include "c_test_common.h"

#include "test_arrays.h"

#ifndef WARMUP
#define WARMUP 1
#endif

#ifndef LOOPS
#define LOOPS 1
#endif

// the driver owns the block loop, so the test names it and not the kernel
#if defined(NF_INT16)
#define FUNC m_txp_i16
#define BLK M_TXP_I16_BLK
#elif defined(NF_INT8)
#define FUNC m_txp_i8
#define BLK M_TXP_I8_BLK
#elif defined(NF_INT4)
#define FUNC m_txp_i4
#define BLK M_TXP_I4_BLK
#elif defined(NF_INT2)
#define FUNC m_txp_i2
#define BLK M_TXP_I2_BLK
#else
_Static_assert(0, "Unsupported number format: FUNC");
#endif

// a partial block has no kernel in any flavor atm, error out
_Static_assert(M % BLK == 0, "M must be a multiple of the block");
_Static_assert(N % BLK == 0, "N must be a multiple of the block");
_Static_assert(LDA >= N, "row stride of the source is shorter than its cols");
_Static_assert(LDC >= M, "row stride of the destination is shorter than its cols");

void main(void) {
    GLOBAL_SYMBOL("warmup");
    for (size_t i = 0; i < WARMUP; i++) {
        FUNC(M, N, a, LDA, c, LDC);
    }
    GLOBAL_SYMBOL("bench");
    PROF_START;
    for (size_t i = 0; i < LOOPS; i++) {
        FUNC(M, N, a, LDA, c, LDC);
    }
    PROF_STOP;

    GLOBAL_SYMBOL("check");
    // 'ref' carries the padding as poison, so comparing the whole destination
    // also catches a kernel writing outside its block
    for (size_t i = 0; i < (sizeof(c) / sizeof(c[0])); i++) {
        CHECK(c[i], ref[i], i + 1); // +1 to avoid writing 0
    }
    pass();
}

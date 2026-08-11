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

// level-2: gemv owns the dimension the caller chose
// picking a kernel is gemv's job, not the test's
#if defined(NF_INT8)
#define FUNC m_gemv_i8_i8
#define KER_MR M_DOTF_I8_I8_MR
#elif defined(NF_INT4_INT8)
#define FUNC m_gemv_i4_i8
#define KER_MR M_DOTF_I4_I8_MR
#elif defined(NF_INT2_INT8)
#define FUNC m_gemv_i2_i8
#define KER_MR M_DOTF_I2_I8_MR
#else
_Static_assert(0, "Unsupported number format: FUNC");
#endif

_Static_assert(N == 1, "gemv writes a single column of outputs");
_Static_assert(LDA >= K, "row stride cannot be shorter than the reduction");
// tails/remainders, remove if used test is used for throughput runs
_Static_assert(M % KER_MR,
    "M must not be a multiple of MR - gemv handles m % MR remainder"
);

void main(void) {
    for (size_t i = 0; i < WARMUP; i++) {
        FUNC(M, VEC_LEN, a, LDA, b, y);
    }
    PROF_START;
    for (size_t i = 0; i < LOOPS; i++) {
        // k is unconstrained, gemv hands it to the kernel which owns its tail
        FUNC(M, VEC_LEN, a, LDA, b, y);
    }
    PROF_STOP;

    for (size_t i = 0; i < M; i++) {
        CHECK(y[i], ref[i], i + 1); // +1 to avoid writing 0
    }
    pass();
}

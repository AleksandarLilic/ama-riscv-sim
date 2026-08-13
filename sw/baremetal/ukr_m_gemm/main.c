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

// level-3: gemm owns both dimensions the caller chose
// picking a kernel is gemm's job, not the test's
#if defined(NF_INT8)
#define FUNC m_gemm_i8_i8
#define KER_MR M_GEMM_I8_I8_MR
#define KER_NR M_GEMM_I8_I8_NR
#define EDGE_MR M_DOTF_I8_I8_MR
#elif defined(NF_INT4_INT8)
#define FUNC m_gemm_i4_i8
#define KER_MR M_GEMM_I4_I8_MR
#define KER_NR M_GEMM_I4_I8_NR
#define EDGE_MR M_DOTF_I4_I8_MR
#elif defined(NF_INT2_INT8)
#define FUNC m_gemm_i2_i8
#define KER_MR M_GEMM_I2_I8_MR
#define KER_NR M_GEMM_I2_I8_NR
#define EDGE_MR M_DOTF_I2_I8_MR
#else
_Static_assert(0, "Unsupported number format: FUNC");
#endif

// both panels are k-contiguous
_Static_assert(LDA >= K,
    "row stride of 'a' cannot be shorter than the reduction"
);
_Static_assert(LDB >= K,
    "row stride of 'b' cannot be shorter than the reduction"
);
// test uses same orientation as MLP so C is (N, M) and its leading dim is M;
// catches a header gen without --c_t, where LDC would be N
_Static_assert(LDC == M, "c_t = true: the outputs' leading dim is M");

#ifndef NO_TAILS
// tails/remainders
_Static_assert(M % KER_MR,
    "M must not be a multiple of MR - gemm handles m % MR remainder"
);
_Static_assert(N % KER_NR,
    "N must not be a multiple of NR - gemm handles n % NR remainder"
);
_Static_assert((M > KER_MR) && (N > KER_NR),
    "shape must leave at least one full MR x NR block"
);
#endif

// the n % NR edge is handed to dotf from inside an MR-blocked loop,
// so the two blockings cannot drift apart
_Static_assert(KER_MR == EDGE_MR, "gemm MR and dotf MR must match");

void main(void) {
    GLOBAL_SYMBOL("warmup");
    for (size_t i = 0; i < WARMUP; i++) {
        FUNC(M, N, VEC_LEN, a, LDA, b, LDB, c, LDC, true);
    }
    GLOBAL_SYMBOL("bench");
    PROF_START;
    for (size_t i = 0; i < LOOPS; i++) {
        // k is unconstrained, gemm hands it to the kernel which owns its tail
        FUNC(M, N, VEC_LEN, a, LDA, b, LDB, c, LDC, true);
    }
    PROF_STOP;

    GLOBAL_SYMBOL("check");
    // C is batch-major, so element (m, n) sits at n*LDC + m
    for (size_t n = 0; n < N; n++) {
        for (size_t m = 0; m < M; m++) {
            const size_t idx = (n * LDC + m);
            CHECK(c[idx], ref[idx], idx + 1); // +1 to avoid writing 0
        }
    }
    pass();
}

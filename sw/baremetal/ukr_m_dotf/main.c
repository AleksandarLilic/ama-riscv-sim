#include <stdint.h>
#include "common.h"
#include "common_math.h"
#include "c_test_common.h"

#include "test_arrays.h"

#ifndef LOOPS
#define LOOPS 1
#endif

// level-1f kernels carry their fusing factor in the name,
// so the test pins the exact kernel instead of going through M_DOTF_*_KER
#if defined(NF_INT8)
#define FUNC m_dotf_i8_i8_mr4
#elif defined(NF_INT4_INT8)
#define FUNC m_dotf_i4_i8_mr4
#elif defined(NF_INT2_INT8)
#define FUNC m_dotf_i2_i8_mr4
#else
_Static_assert(0, "Unsupported number format: FUNC");
#endif

_Static_assert(M == 4, "M must match the fusing factor of kernel under test");
_Static_assert(N == 1, "dotf writes a single column of outputs");
_Static_assert(LDA >= K, "row stride cannot be shorter than the reduction");

void main(void) {
    PROF_START;
    for (size_t i = 0; i < LOOPS; i++) {
        FUNC(VEC_LEN, a, LDA, b, y);
    }
    PROF_STOP;

    for (size_t i = 0; i < M; i++) {
        CHECK(y[i], ref[i], i + 1); // +1 to avoid writing 0
    }
    pass();
}

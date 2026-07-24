#include <stdint.h>
#include "common.h"
#include "common_math.h"

#include "test_arrays.h"

#ifndef LOOPS
#define LOOPS 1
#endif

// function
#ifdef __riscv_xsimd
#define FUNC_PREFIX _simd_
#else
#define FUNC_PREFIX
#endif

#if defined(NF_INT16)
#define FUNC_NAME dot_product_int16
#elif defined(NF_INT8)
#define FUNC_NAME dot_product_int8
#elif defined(NF_INT4)
#define FUNC_NAME dot_product_int4
#elif defined(NF_INT2)
#define FUNC_NAME dot_product_int2
#elif defined(NF_INT16_INT8)
#define FUNC_NAME dot_product_int16_int8
#elif defined(NF_INT16_INT4)
#define FUNC_NAME dot_product_int16_int4
#elif defined(NF_INT16_INT2)
#define FUNC_NAME dot_product_int16_int2
#elif defined(NF_INT8_INT4)
#define FUNC_NAME dot_product_int8_int4
#elif defined(NF_INT8_INT2)
#define FUNC_NAME dot_product_int8_int2
#elif defined(NF_INT4_INT2)
#define FUNC_NAME dot_product_int4_int2
#else
_Static_assert(0, "Unsupported number format: FUNC_NAME");
#endif

#define CONCAT(a, b) a##b
#define EXPAND_CONCAT(a, b) CONCAT(a, b)
#define FUNC EXPAND_CONCAT(FUNC_PREFIX, FUNC_NAME)

void main(void) {
    int32_t result;
    PROF_START;
    for (size_t i = 0; i < LOOPS; i++) {
        result = FUNC(a, b, VEC_LEN);
    }
    PROF_STOP;
    if (result != ref) {
        write_mismatch(result, ref, 1);
        fail();
    }
    pass();
}

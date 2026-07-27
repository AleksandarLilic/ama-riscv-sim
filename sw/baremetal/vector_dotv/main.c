#include <stdint.h>
#include "common.h"
#include "common_math.h"

#include "test_arrays.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#if defined(NF_INT16)
#define FUNC m_dotv_i16_i16
#elif defined(NF_INT8)
#define FUNC m_dotv_i8_i8
#elif defined(NF_INT4)
#define FUNC m_dotv_i4_i4
#elif defined(NF_INT2)
#define FUNC m_dotv_i2_i2
#elif defined(NF_INT16_INT8)
#define FUNC m_dotv_i16_i8
#elif defined(NF_INT16_INT4)
#define FUNC m_dotv_i16_i4
#elif defined(NF_INT16_INT2)
#define FUNC m_dotv_i16_i2
#elif defined(NF_INT8_INT4)
#define FUNC m_dotv_i8_i4
#elif defined(NF_INT8_INT2)
#define FUNC m_dotv_i8_i2
#elif defined(NF_INT4_INT2)
#define FUNC m_dotv_i4_i2
#else
_Static_assert(0, "Unsupported number format: FUNC");
#endif

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

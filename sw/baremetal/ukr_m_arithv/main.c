#include "common.h"
#include "common_math.h"

#ifndef WARMUP
#define WARMUP 1
#endif

#ifndef LOOPS
#define LOOPS 1
#endif

#if defined(OP_ADD)
#include "test_arrays_add.h"
#elif defined(OP_SUB)
#include "test_arrays_sub.h"
#elif defined(OP_WMUL)
#include "test_arrays_wmul.h"
#elif defined(OP_DIV)
#include "test_arrays_div.h"
#else
_Static_assert(0, "No operation defined");
#endif

#if defined(OP_ADD)
    #if defined(NF_INT16) || defined(NF_UINT16)
        #define FUNC m_add_i16
    #elif defined(NF_INT8) || defined(NF_UINT8)
        #define FUNC m_add_i8
    #else
        #define NO_VEC
    #endif

#elif defined(OP_SUB)
    #if defined(NF_INT16) || defined(NF_UINT16)
        #define FUNC m_sub_i16
    #elif defined(NF_INT8) || defined(NF_UINT8)
        #define FUNC m_sub_i8
    #else
        #define NO_VEC
    #endif

#elif defined(OP_WMUL)
    #if defined(NF_INT16)
        #define FUNC m_wmul_i16
    #elif defined(NF_UINT16)
        #define FUNC m_wmul_u16
    #elif defined(NF_INT8)
        #define FUNC m_wmul_i8
    #elif defined(NF_UINT8)
        #define FUNC m_wmul_u8
    #else
        #define NO_VEC
    #endif

#elif defined(OP_DIV)
    #define NO_VEC // no vector routines for div

#endif // OP_*

#ifdef NO_VEC
// generic scalar version
#define RUN for (size_t k = 0; k < ARR_LEN; k++) c[k] = a[k] OP b[k];
#else
// SIMD version where supported
#define RUN FUNC(a, b, c, ARR_LEN);
#endif

void main(void) {
    GLOBAL_SYMBOL("warmup");
    for (uint32_t i = 0; i < WARMUP; i++) {
        RUN
    }
    GLOBAL_SYMBOL("bench");
    PROF_START;
    for (uint32_t i = 0; i < LOOPS; i++) {
        RUN
    }
    PROF_STOP;

    GLOBAL_SYMBOL("check");
    for (size_t j = 0; j < ARR_LEN; j++) {
        //printf("c[%d] = %d, ref[%d] = %d\n", j, c[j], j, ref[j]);
        if (c[j] != ref[j]) {
            write_mismatch(c[j], ref[j], j+1); // +1 to avoid writing 0
            fail();
        }
    }
    pass();
}

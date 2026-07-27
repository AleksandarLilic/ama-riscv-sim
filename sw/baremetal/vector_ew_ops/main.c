#include "common.h"
#include "common_math.h"

#if defined(OP_ADD)
#include "test_arrays_add.h"
#elif defined(OP_SUB)
#include "test_arrays_sub.h"
#elif defined(OP_MUL)
#include "test_arrays_mul.h"
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

#elif defined(OP_MUL)
    #if defined(NF_INT16)
        #define FUNC m_mul_i16
    #elif defined(NF_UINT16)
        #define FUNC m_mul_u16
    #elif defined(NF_INT8)
        #define FUNC m_mul_i8
    #elif defined(NF_UINT8)
        #define FUNC m_mul_u8
    #else
        #define NO_VEC
    #endif

#elif defined(OP_DIV)
    #define NO_VEC // no vector routines for div

#endif // OP_*

#ifndef LOOPS
#define LOOPS 1u
#endif

void set_c() {
    for (size_t i = 0; i < ARR_LEN; i++) c[i] = 0;
}

void main(void) {
    for (uint32_t i = 0; i < LOOPS; i++) {
        set_c();

        asm(".global compute");
        asm("compute:");
        PROF_START;

        #ifdef NO_VEC
        // generic scalar version
        for (size_t k = 0; k < ARR_LEN; k++) c[k] = a[k] OP b[k];
        #else
        // SIMD version where supported
        FUNC(a, b, c, ARR_LEN);
        #endif

        PROF_STOP;
        asm(".global check");
        asm("check:");
        for (size_t j = 0; j < ARR_LEN; j++) {
            //printf("c[%d] = %d, ref[%d] = %d\n", j, c[j], j, ref[j]);
            if (c[j] != ref[j]) {
                write_mismatch(c[j], ref[j], j+1); // +1 to avoid writing 0
                fail();
            }
        }
    }
    pass();
}

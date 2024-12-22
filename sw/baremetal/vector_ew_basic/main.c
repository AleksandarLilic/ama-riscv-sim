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

// function
#ifdef CUSTOM_ISA
#define FUNC_PREFIX _simd_
#else
#define FUNC_PREFIX
#endif

#if defined(OP_ADD)
    #if defined(NF_INT16) || defined(NF_UINT16)
        #define FUNC_NAME add_int16
    #elif defined(NF_INT8) || defined(NF_UINT8)
        #define FUNC_NAME add_int8
    #else
        #define NO_SIMD
    #endif

#elif defined(OP_SUB)
    #if defined(NF_INT16) || defined(NF_UINT16)
        #define FUNC_NAME sub_int16
    #elif defined(NF_INT8) || defined(NF_UINT8)
        #define FUNC_NAME sub_int8
    #else
        #define NO_SIMD
    #endif

#elif defined(OP_MUL)
    #if defined(NF_INT16)
        #define FUNC_NAME mul_int16
    #elif defined(NF_UINT16)
        #define FUNC_NAME mul_uint16
    #elif defined(NF_INT8)
        #define FUNC_NAME mul_int8
    #elif defined(NF_UINT8)
        #define FUNC_NAME mul_uint8
    #else
        #define NO_SIMD
    #endif

#elif defined(OP_DIV)
    #define NO_SIMD // no SIMD support for div

#endif // OP_ADD

#define CONCAT(a, b) a##b
#define EXPAND_CONCAT(a, b) CONCAT(a, b)
#define FUNC EXPAND_CONCAT(FUNC_PREFIX, FUNC_NAME)

#define LOOP_COUNT 1u

void set_c() {
    for (size_t i = 0; i < ARR_LEN; i++) c[i] = 0;
}

void main(void) {
    for (uint32_t i = 0; i < LOOP_COUNT; i++) {
        set_c();

        asm(".global compute");
        asm("compute:");
        LOG_START;

        #ifdef NO_SIMD
        // generic scalar version
        for (size_t k = 0; k < ARR_LEN; k++) c[k] = a[k] OP b[k];
        #else
        // SIMD version where supported
        FUNC(a, b, c, ARR_LEN);
        #endif

        LOG_STOP;
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

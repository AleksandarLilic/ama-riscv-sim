#include <stdint.h>
#include "common.h"
#include "common_math.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#define ARR_LEN 16
// input data A
#if defined(NF_INT16) || defined(NF_INT16_INT8) || defined(NF_INT16_INT4)
int16_t a[ARR_LEN] = {
    -30036, 10799, 9845, 19648, 1, -11525, -2365, 1,
    9225, 24275, 1, 1, 14116, -17833, -17338, 15832 };
#elif defined(NF_INT8) || defined(NF_INT8_INT4)
int8_t a[ARR_LEN] = {
    44, -81, -11, 64, -61, 123, 67, -25, -119, 83, -107, 114, -92, -41, -58, 88
};
#elif defined(NF_INT4)
// packed data type, has to be a multiple of 2 (pad with 0 at the end if needed)
int8_t a[ARR_LEN>>1] = { 116, -115, 59, -5, -79, -83, -4, 14 };
#else
_Static_assert(0, "Unsupported number format");
#endif

// input data B
#if defined(NF_INT16)
int16_t b[ARR_LEN] = {
    6744, 19852, -18118, -15679, -538, -1, 10327, 18606,
    -8616, 2897, 26277, -1, -1, 6216, -1, 6036 };
#elif defined(NF_INT16_INT8) || defined(NF_INT8)
int8_t b[ARR_LEN] = {
    -40, 12, -70, 65, 102, -89, -41, 46, -40, -47, 37, -103, -51, -56, -119, 20
};
#elif defined(NF_INT16_INT4) || defined(NF_INT8_INT4) || defined(NF_INT4)
int8_t b[ARR_LEN>>1] = { 64, -110, -2, 111, -112, 29, 5, -63 };
#else
_Static_assert(0, "Unsupported number format");
#endif

// function
#ifdef CUSTOM_ISA
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
#elif defined(NF_INT16_INT8)
#define FUNC_NAME dot_product_int16_int8
#elif defined(NF_INT16_INT4)
#define FUNC_NAME dot_product_int16_int4
#elif defined(NF_INT8_INT4)
#define FUNC_NAME dot_product_int8_int4
#else
_Static_assert(0, "Unsupported number format");
#endif

#define CONCAT(a, b) a##b
#define EXPAND_CONCAT(a, b) CONCAT(a, b)
#define FUNC EXPAND_CONCAT(FUNC_PREFIX, FUNC_NAME)

// reference output
#if defined(NF_INT16)
const int32_t ref = -523423903;
#elif defined(NF_INT8)
const int32_t ref = -18060;
#elif defined(NF_INT4)
const int32_t ref = 100;
#elif defined(NF_INT16_INT8)
const int32_t ref = 4190439;
#elif defined(NF_INT16_INT4)
const int32_t ref = -240769;
#elif defined(NF_INT8_INT4)
const int32_t ref = -2028;
#else
_Static_assert(0, "Unsupported number format");
#endif

void main(void) {
    for (size_t i = 0; i < LOOPS; i++) {
        LOG_START;
        int32_t result = FUNC(a, b, ARR_LEN);
        LOG_STOP;
        printf("%d\n",result);
        printf("%d\n",ref);
        if (result != ref) {
            write_mismatch(result, ref, 1);
            fail();
        }
    }
    pass();
}

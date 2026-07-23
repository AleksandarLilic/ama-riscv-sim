#include <stdint.h>
#include "common.h"
#include "common_math.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#define ARR_LEN 24
// packed data type has to within 1 byte exactly

// input data A
#if defined(NF_INT16) || defined(NF_INT16_INT8) || defined(NF_INT16_INT4) || defined(NF_INT16_INT2)
int16_t a[ARR_LEN] = {
    29733, 235, 1, -27576, -257, 17289, 10955, -24955,
    19279, -11328, 144, 1, 1, -25017, 10989, -1540,
    3462, -743, 1, 21780, -11010, 12645, 2962, 18900
};
#elif defined(NF_INT8) || defined(NF_INT8_INT4) || defined(NF_INT8_INT2)
int8_t a[ARR_LEN] = {
    -91, 107, 12, -56, 127, 9, 75, 5,
    -49, 64, 16, 1, 76, -57, 109, 124,
    6, -103, 50, -108, 126, -27, 18, 84
};
#elif defined(NF_INT4) || defined(NF_INT4_INT2)
int8_t a[ARR_LEN>>1] = {
    61, 4, 23, -45, -121, -104, -12, 69, 30, -54, -42, -54
};
/* actual values {
    (-3, 3), (4, 0), (7, 1), (3, -3),
    (7, -8), (-8, -7), (4, -1), (5, 4),
    (-2, 1), (-6, -4), (6, -3), (-6, -4)
}; */
#elif defined(NF_INT2)
int8_t a[ARR_LEN>>2] = { -89, -35, -23, -74, -116, -116 };
/* actual values {
    (-1, 1, -2, -2), (1, -1, 1, -1), (1, -2, -2, -1), (-2, 1, -1, -2),
    (0, -1, 0, -2), (0, -1, 0, -2)
}; */
#else
_Static_assert(0, "Unsupported number format");
#endif

// input data B
#if defined(NF_INT16)
int16_t b[ARR_LEN] = {
    31627, -24324, -12822, 4764, 24477, -9842, 16946, -5564,
    -809, -8233, -13335, -1, -1, 11742, -1, 1110,
    6285, -1, 32137, 1031, 4415, 19261, -29162, 9529
};
#elif defined(NF_INT16_INT8) || defined(NF_INT8)
int8_t b[ARR_LEN] = {
    11, 124, 106, 28, 29, 14, -78, -60,
    87, 87, 105, 113, 119, 94, -32, -42,
    13, 105, 9, -121, -65, -67, -106, -71
};
#elif defined(NF_INT16_INT4) || defined(NF_INT8_INT4) || defined(NF_INT4)
int8_t b[ARR_LEN>>1] = {
    67, 66, 101, -54, -1, -111, 111, -24, 21, -15, 87, 30
};
/* actual values {
    (3, 4), (2, 4), (5, 6), (-6, -4),
    (-1, -1), (1, -7), (-1, 6), (-8, -2),
    (5, 1), (1, -1), (7, 5), (-2, 1)
}; */
#elif defined(NF_INT16_INT2) || defined(NF_INT8_INT2) || defined(NF_INT4_INT2) || defined(NF_INT2)
int8_t b[ARR_LEN>>2] = { -119, -125, -11, 33, 127, -51 };
/* actual values {
    (1, -2, 0, -2), (-1, 0, 0, -2), (1, 1, -1, -1), (1, 0, -2, 0),
    (-1, -1, -1, 1), (1, -1, 0, -1 )
}; */
#else
_Static_assert(0, "Unsupported number format");
#endif

// function
#ifdef __riscv_xsimd
#define FUNC_PREFIX _simd_
#else
#define FUNC_PREFIX
#endif

#if defined(NF_INT16)
#define FUNC_NAME dot_product_int16
const int32_t ref = 884400237;
const size_t len = 23;
#elif defined(NF_INT8)
#define FUNC_NAME dot_product_int8
const int32_t ref = 2210;
const size_t len = 23;
#elif defined(NF_INT4)
#define FUNC_NAME dot_product_int4
const int32_t ref = 46;
const size_t len = 22;
#elif defined(NF_INT2)
#define FUNC_NAME dot_product_int2
const int32_t ref = 3;
const size_t len = 20;
#elif defined(NF_INT16_INT8)
#define FUNC_NAME dot_product_int16_int8
const int32_t ref = -4583810;
const size_t len = 23;
#elif defined(NF_INT16_INT4)
#define FUNC_NAME dot_product_int16_int4
const int32_t ref = -145430;
const size_t len = 22;
#elif defined(NF_INT16_INT2)
#define FUNC_NAME dot_product_int16_int2
const int32_t ref = 139471;
const size_t len = 20;
#elif defined(NF_INT8_INT4)
#define FUNC_NAME dot_product_int8_int4
const int32_t ref = -538;
const size_t len = 22;
#elif defined(NF_INT8_INT2)
#define FUNC_NAME dot_product_int8_int2
const int32_t ref = -535;
const size_t len = 20;
#elif defined(NF_INT4_INT2)
#define FUNC_NAME dot_product_int4_int2
const int32_t ref = 1;
const size_t len = 20;
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
        result = FUNC(a, b, len);
    }
    PROF_STOP;
    //printf("%d\n",result);
    //printf("%d\n",ref);
    if (result != ref) {
        write_mismatch(result, ref, 1);
        fail();
    }
    pass();
}

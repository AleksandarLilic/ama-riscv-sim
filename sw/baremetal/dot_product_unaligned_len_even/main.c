#include <stdint.h>
#include "common.h"
#include "common_math.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#define ARR_LEN 22

// input data A
#if defined(NF_INT16) || defined(NF_INT16_INT8) || defined(NF_INT16_INT4)
int16_t a[ARR_LEN] = {
    29733, 235, 1, -27576, -257, 17289, 10955, -24955,
    19279, -11328, 144, -12159, 1, -25017, 1, -1540,
    1, -743, 25778, 21780, -11010, 12645 };
#elif defined(NF_INT8) || defined(NF_INT8_INT4)
int8_t a[ARR_LEN] = {
    -91, 107, 12, -56, 127, 9, 75, 5,
    -49, 64, 16, 1, 76, -57, 109, 124,
    6, -103, 50, -108, 126, -27 };
#elif defined(NF_INT4)
// packed data type, has to be a multiple of 2 (or padded with 0 at the end)
int8_t a[ARR_LEN>>1] = { 61, 4, 23, -45, -121, -104, -12, 69, 30, -54, -42 };
/* actual values
{ -3, 3, 4, 0, 7, 1, 3, -3,
   7, -8, -8, -7, 4, -1, 5, 4,
   -2, 1, -6, -4, 6, -3 };
*/
#else
_Static_assert(0, "Unsupported number format");
#endif

// input data B
#if defined(NF_INT16)
int16_t b[ARR_LEN] = {
    2962, 18900, 31627, -24324, -12822, -1, 24477, -9842,
    16946, -5564, -809, -1, -13335, -15631, -6665, 11742,
    18272, 1110, 6285, -1, 32137, -1 };
#elif defined(NF_INT16_INT8) || defined(NF_INT8)
int8_t b[ARR_LEN] = {
    18, 84, 11, 124, 106, 28, 29, 14,
    -78, -60, 87, 87, 105, 113, 119, 94,
    -32, -42, 13, 105, 9, -121 };
#elif defined(NF_INT16_INT4) || defined(NF_INT8_INT4) || defined(NF_INT4)
int8_t b[ARR_LEN>>1] = { -54, 67, 66, 101, -54, -1, -111, 111, -24, 21, -15 };
/* actual values
{ -6, -4, 3, 4, 2, 4, 5, 6,
  -6, -4, -1, -1, 1, -7, -1, 6,
  -8, -2, 5, 1, 1, -1 };
*/
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
const int32_t ref = 1850241620;
#elif defined(NF_INT8)
const int32_t ref = 41969;
#elif defined(NF_INT4)
const int32_t ref = 57;
#elif defined(NF_INT16_INT8)
const int32_t ref = -6256095;
#elif defined(NF_INT16_INT4)
const int32_t ref = -79927;
#elif defined(NF_INT8_INT4)
const int32_t ref = 2209;
#else
_Static_assert(0, "Unsupported number format");
#endif

void main(void) {
    for (size_t i = 0; i < LOOPS; i++) {
        int32_t result = FUNC(a, b, ARR_LEN);
        //printf("%d\n",result);
        //printf("%d\n",ref);
        if (result != ref) {
            write_mismatch(result, ref, 1);
            fail();
        }
    }
    pass();
}

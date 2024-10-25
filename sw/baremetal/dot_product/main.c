#include <stdint.h>
#include "common.h"
#include "common_math.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#if defined(NF_INT16)
#define ARR_LEN 16
#define FUNC dot_product_int16
int16_t a[ARR_LEN] = {
    -30036, 10799, 9845, 19648, 13123, -11525, -2365, -665,
    9225, 24275, -12011, 22258, 14116, -17833, -17338, 15832 };
int16_t b[ARR_LEN] = {
    6744, 19852, -18118, -15679, -538, -13785, 10327, 18606,
    -8616, 2897, 26277, -15847, -5299, 6216, -25847, 6036 };
const int32_t ref = -679045004;

#elif defined(NF_INT8)
#define FUNC dot_product_int8
#define ARR_LEN 16
int8_t a[ARR_LEN] = {
    44, -81, -11, 64, -61, 123, 67, -25, -119, 83, -107, 114, -92, -41, -58, 88
};
int8_t b[ARR_LEN] = {
    -40, 12, -70, 65, 102, -89, -41, 46, -40, -47, 37, -103, -51, -56, -119, 20
};
const int32_t ref = -18060;

#elif defined(NF_INT4)
#define FUNC dot_product_int4
#define ARR_LEN 8 // pairs of 4-bit numbers
int8_t a[ARR_LEN] = { -44, -72, -77, 31, -37, -54, -17, 0 };
int8_t b[ARR_LEN] = { 36, -23, -1, 6, -39, 81, 16, -68 };
// a = {4, -3, -8, -5, 3, -5, -1, 1, -5, -3, -6, -4, -1, -2, 0, 0}; // as 4-bit
// b = {4, 2, -7, -2, -1, -1, 6, 0, -7, -3, 1, 5, 0, 1, -4, -5}; // as 4-bit
const int32_t ref = 88;

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

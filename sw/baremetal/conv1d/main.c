#include <stdint.h>
#include "common.h"
#include "common_math.h"

#ifndef LOOPS
#define LOOPS 1
#endif

#define IN_LEN 16
#define F_LEN 5
#define OUT_LEN IN_LEN - F_LEN + 1

#if defined(NF_INT16)
int16_t in[IN_LEN] = {
    -30036, 10799, 9845, 19648, 1, -11525, -2365, 1,
    9225, 24275, 1, 1, 14116, -17833, -17338, 15832 };
int16_t filter[F_LEN] = { 6744, 19852, -18118, -15679, -538 };
int32_t ref[OUT_LEN] = {
    -474614276, -81526297, 638401503, 378416211, -190917215, -282391423,
    -563676521, -256689223, 536492495, -48018276, 33204359, 866660756 };
int32_t out[OUT_LEN];
#else
_Static_assert(0, "Unsupported number format");
#endif

void conv1d_int16(
    const int16_t *in, size_t in_size,
    const int16_t *filter, size_t filter_size,
    int32_t *out)
{
    size_t out_size = in_size - filter_size + 1;

    // aligned copy for SIMD even iterations
    int16_t filter2[filter_size];
    for (size_t i = 0; i < filter_size-1; i++) filter2[i] = filter[i+1];
    filter2[filter_size] = 0;

    const int16_t *filter_ptr = filter;
    const int16_t *filter2_ptr = filter2;
    int32_t sum = 0;
    for (size_t i = 0; i < out_size; i+=2) {
        const int16_t *in_ptr = in + i;
        sum = _simd_dot_product_int16(in_ptr, filter_ptr, filter_size);
        *(out + i) = sum;

        const int16_t *in_ptr2 = in + i + 1;
        sum = *(in_ptr2) * *(filter_ptr); // handle unaligned input element

        sum += _simd_dot_product_int16(in_ptr2+1, filter2_ptr, filter_size-1);
        *(out + i + 1) = sum;
    }
}

void main(void) {
    for (size_t i = 0; i < LOOPS; i++) {
        LOG_START;
        conv1d_int16(in, IN_LEN, filter, F_LEN, out);
        LOG_STOP;

        for (size_t j = 0; j < OUT_LEN; j++) {
            //printf("out[%d] = %d, ref[%d] = %d\n", j, out[j], j, ref[j]);
            if (out[j] != ref[j]) {
                write_mismatch(out[j], ref[j], j+1); // +1 to avoid writing 0
                fail();
            }
        }
    }
    pass();
}

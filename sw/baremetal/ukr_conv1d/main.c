#include <stdint.h>
#include "common.h"
#include "common_math.h"

#ifndef WARMUP
#define WARMUP 1
#endif

#ifndef LOOPS
#define LOOPS 1
#endif

#ifdef ALIGNED
#define ALIGN __attribute__((aligned(CACHE_LINE_SIZE)))
#else
#define ALIGN
#endif

#if defined(NF_INT16)
#include "test_arrays.h"
#else
_Static_assert(0, "Unsupported number format");
#endif

#define ROUND_UP(a, len) ((((a) + ((len) - 1)) / (len)) * (len))
#define ROUND_DOWN(a, len) (((a) / (len)) * (len))

// round up to a whole SIMD width (VL) of filter elements
#define FP_LEN ROUND_UP((F_LEN + VL - 1), VL)

int32_t out[OUT_LEN];
// VL num of phases, aligned filter copy for each
int16_t fp[VL][FP_LEN] __attribute__((aligned(4))); // zero-initialized

void generate_filters() {
    for (size_t p = 0; p < VL; p++) {
        for (size_t m = 0; m < F_LEN; m++) { // everything else stays 0
            fp[p][p + m] = f[m];
        }
    }
}

void conv1d_int16(
    const int16_t *in, size_t in_len,
    const int16_t *fp, size_t fp_len,
    int32_t *out, size_t out_len)
{
    const size_t vl = 2; // vector length of 2 for i16, self contained version
    // how far the fast loop may go, one limit per array, write and read guards
    const size_t rem = (out_len % vl); // last partial group can't do entire vl
    const size_t lim_wr = (out_len - rem); // last whole group out[] can hold
    const size_t lim_rd = (in_len >= fp_len) ? // last whole group in[] can feed
        (ROUND_DOWN(in_len - fp_len, vl) + vl) : 0;
    const size_t n_fast = minu(lim_rd, lim_wr);

    const int16_t *a = in;
    for (size_t n = 0; n < n_fast; n += vl) {
        for (size_t p = 0; p < vl; p++) { // phase
            out[n + p] = m_dotv_i16_i16(a, (fp + (p * fp_len)), fp_len);
        }
        a += vl;
    }

    // NOTE: if input is guaranteed to be `IN_LEN + FP_LEN - F_LEN` long,
    // the below `k` shortening is not needed as input can't be over-read
    // (the excess length need not be even initialized, fp is zero there)

    // tail, one output at a time
    for (size_t n = n_fast; n < out_len; n++) {
        const size_t p = (n % vl); // phase
        const int16_t *a = (in + (n - p)); // input, aligned
        const size_t k = (in_len - (n - p)); // k, shortened as needed
        out[n] = m_dotv_i16_i16(a, (fp + (p * fp_len)), k);
    }
}

void main(void) {
    GLOBAL_SYMBOL("filter_gen");
    // depending on the profiling goal,
    // filter gen can be done once at the beginning or on each call
    generate_filters();

    GLOBAL_SYMBOL("warmup");
     for (size_t i = 0; i < WARMUP; i++) {
        //SCP_LCL(in);
        conv1d_int16(in, IN_LEN, &fp[0][0], FP_LEN, out, OUT_LEN);
        //SCP_REL(in);
    }
    GLOBAL_SYMBOL("bench");
    PROF_START;
    for (size_t i = 0; i < LOOPS; i++) {
        //SCP_LCL(in);
        conv1d_int16(in, IN_LEN, &fp[0][0], FP_LEN, out, OUT_LEN);
        //SCP_REL(in);
    }
    PROF_STOP;

    GLOBAL_SYMBOL("check");
    for (size_t j = 0; j < OUT_LEN; j++) {
        //printf("out[%d] = %d, ref[%d] = %d\n", j, out[j], j, ref[j]);
        if (out[j] != ref[j]) {
            write_mismatch(out[j], ref[j], j+1); // +1 to avoid writing 0
            fail();
        }
    }
    pass();
}

#include "common_math.h"
#include "common_math_dotv_scalar_core.h"
#include "common_math_simd_intrinsics.h"
#include "common_math_simd_v_load.h"

#ifdef __riscv_xsimd

// these have the 'unrolled' optimized option, so '_core' is provided for
// 1. (#ifdef SIMD_UNROLL) last step if inputs are not multiple of tile size or
// 2. (#else) indirection in the regular version
static INLINE int32_t m_dotv_i8_i8_simd_core(
    const int8_t* a, const int8_t* b, const size_t len
);
static INLINE int32_t m_dotv_i8_i4_simd_core(
    const int8_t* a, const int8_t* b, const size_t len
);
static INLINE int32_t m_dotv_i8_i2_simd_core(
    const int8_t* a, const int8_t* b, const size_t len
);

INLINE_OPTION
int32_t m_dotv_i16_i16(
    const int16_t* a, const int16_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s2 = ((len >> 1) << 1);
    for (size_t k = 0; k < len_s2; k += 2) {
        const int16x2_t a_slice = v_load_int16x2(a + k);
        const int16x2_t b_slice = v_load_int16x2(b + k);
        _dot16(a_slice, b_slice, &c);
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        c += m_dotv_i16_i16_scalar_core(a + len_s2, b + len_s2, rem);
    }
    return c;
}

#ifdef SIMD_UNROLL
INLINE_OPTION
int32_t m_dotv_i8_i8(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 3; // unroll degree
    static const size_t deg = (2 + udeg); // +2 for bytes to words
    size_t tile = ((len >> deg) << deg); // +2 to words, +3 for 8x unroll

    for (size_t k = 0; k < tile; k += (1 << deg)) {
        int8x4_t a_arr[8], b_arr[8]; // 8=(1<<udeg), but compiler is not happy

        #pragma GCC unroll 8
        for (size_t i = 0; i < 8; i++) {
            a_arr[i] = v_load_int8x4(a + k + i * 4);
            b_arr[i] = v_load_int8x4(b + k + i * 4);
        }
        asm volatile (
            "dot8 %[c], %[a1], %[b1]\n\t"
            "dot8 %[c], %[a2], %[b2]\n\t"
            "dot8 %[c], %[a3], %[b3]\n\t"
            "dot8 %[c], %[a0], %[b0]\n\t" // scheduling, b0 often loaded last
            : [c] "+r" (c)
            : [a0] "r" (a_arr[0]), [b0] "r" (b_arr[0]),
              [a1] "r" (a_arr[1]), [b1] "r" (b_arr[1]),
              [a2] "r" (a_arr[2]), [b2] "r" (b_arr[2]),
              [a3] "r" (a_arr[3]), [b3] "r" (b_arr[3])
            :
        );
        // let compiler schedule loads in between to reduce rf pressure
        asm volatile (
            "dot8 %[c], %[a5], %[b5]\n\t"
            "dot8 %[c], %[a6], %[b6]\n\t"
            "dot8 %[c], %[a7], %[b7]\n\t"
            "dot8 %[c], %[a4], %[b4]\n\t"
            : [c] "+r" (c)
            : [a4] "r" (a_arr[4]), [b4] "r" (b_arr[4]),
              [a5] "r" (a_arr[5]), [b5] "r" (b_arr[5]),
              [a6] "r" (a_arr[6]), [b6] "r" (b_arr[6]),
              [a7] "r" (a_arr[7]), [b7] "r" (b_arr[7])
            :
        );
    }

    // large tiles exhausted, finish with the regular SIMD core
    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i8_i8_simd_core(a + tile, b + tile, rem);
    }
    return c;
}

#else
INLINE_OPTION
int32_t m_dotv_i8_i8(
    const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i8_i8_simd_core(a, b, len);
}

#endif // SIMD_UNROLL

static INLINE
int32_t m_dotv_i8_i8_simd_core(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s4 = ((len >> 2) << 2);
    for (size_t k = 0; k < len_s4; k += 4) {
        const int8x4_t a_slice = v_load_int8x4(a + k);
        const int8x4_t b_slice = v_load_int8x4(b + k);
        _dot8(a_slice, b_slice, &c);
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        c += m_dotv_i8_i8_scalar_core(a + len_s4, b + len_s4, rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i4_i4(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_bytes = (len >> 1); // len passed in as number of nibbles
    size_t len_s4 = ((len_bytes) >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int4x8_t a_slice = v_load_int4x8(a + k);
        const int4x8_t b_slice = v_load_int4x8(b + k);
        _dot4(a_slice, b_slice, &c);
    }
    size_t rem = (len_bytes - len_s4);
    if (rem > 0) {
        c += m_dotv_i4_i4_scalar_core(a + len_s4, b + len_s4, rem << 1);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i2_i2(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_bytes = (len >> 2); // len passed in as number of crumbs
    size_t len_s4 = ((len_bytes) >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int2x16_t a_slice = v_load_int2x16(a + k);
        const int2x16_t b_slice = v_load_int2x16(b + k);
        _dot2(a_slice, b_slice, &c);
    }
    size_t rem = (len_bytes - len_s4);
    if (rem > 0) {
        c += m_dotv_i2_i2_scalar_core(a + len_s4, b + len_s4, rem << 2);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i8(
    const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s4 = ((len >> 2) << 2);
    for (size_t k = 0; k < len_s4; k += 4) {
        const int8x4_t b_slice = v_load_int8x4(b + k);
        const int16x2_t a_slice_1 = v_load_int16x2(a + k);
        const int16x2_t a_slice_2 = v_load_int16x2(a + k + 2);
        const int16x4_t b_slice_wide = _widen8(b_slice, 0u);
        asm volatile (
            "dot16 %[c], %[a1], %[bw_lo]\n\t"
            "dot16 %[c], %[a2], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide.w.lo), [bw_hi] "r" (b_slice_wide.w.hi),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
        );
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        c += m_dotv_i16_i8_scalar_core(a + len_s4, b + len_s4, rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i4(
    const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s8 = ((len >> 3) << 3);
    for (size_t k = 0; k < len_s8; k += 8) {
        const int4x8_t b_slice = v_load_int4x8(b + (k >> 1));
        const int16x2_t a_slice_1 = v_load_int16x2(a + k);
        const int16x2_t a_slice_2 = v_load_int16x2(a + k + 2);
        const int16x2_t a_slice_3 = v_load_int16x2(a + k + 4);
        const int16x2_t a_slice_4 = v_load_int16x2(a + k + 6);
        int8x8_t b_slice_wide_b;
        int16x4_t b_slice_wide_h;
        b_slice_wide_b = _widen4(b_slice, 0u); // low N to B
        b_slice_wide_h = _widen8(b_slice_wide_b.w.lo, 0u); // low B to H
        asm volatile (
            "dot16 %[c], %[a1], %[bw_lo]\n\t"
            "dot16 %[c], %[a2], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide_h.w.lo),
              [bw_hi] "r" (b_slice_wide_h.w.hi),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
        );
        b_slice_wide_h = _widen8(b_slice_wide_b.w.hi, 0u); // high B to H
        asm volatile (
            "dot16 %[c], %[a3], %[bw_lo]\n\t"
            "dot16 %[c], %[a4], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide_h.w.lo),
              [bw_hi] "r" (b_slice_wide_h.w.hi),
              [a3] "r" (a_slice_3), [a4] "r" (a_slice_4)
        );
    }
    size_t rem = (len - len_s8);
    if (rem > 0) {
        c += m_dotv_i16_i4_scalar_core(a + len_s8, b + (len_s8 >> 1), rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i2(
    const int16_t* a, const int8_t* b, const size_t len)
{

    #define DOT16_BLOCK_2() \
        asm volatile ( \
            "dot16 %[c], %[a1], %[bw_lo]\n\t" \
            "dot16 %[c], %[a2], %[bw_hi]\n\t" \
            : [c] "+r" (c) \
            : [a1] "r" (a_slice_1), [a2] "r" (a_slice_2), \
              [bw_lo] "r" (b_slice_wide_h.w.lo), \
              [bw_hi] "r" (b_slice_wide_h.w.hi) \
        );

    int32_t c = 0;
    size_t len_s16 = ((len >> 4) << 4);
    for (size_t k = 0; k < len_s16; k += 16) {
        const int2x16_t b_slice = v_load_int2x16(b + (k >> 2));
        int4x16_t b_slice_wide_n;
        int8x8_t b_slice_wide_b;
        int16x4_t b_slice_wide_h;
        int16x2_t a_slice_1, a_slice_2;

        a_slice_1 = v_load_int16x2(a + k);
        a_slice_2 = v_load_int16x2(a + k + 2);

        b_slice_wide_n = _widen2(b_slice, 0u); // C to N
        b_slice_wide_b = _widen4(b_slice_wide_n.w.lo, 0u); // low N to B
        b_slice_wide_h = _widen8(b_slice_wide_b.w.lo, 0u); // low B to H
        DOT16_BLOCK_2()

        a_slice_1 = v_load_int16x2(a + k + 4);
        a_slice_2 = v_load_int16x2(a + k + 6);
        b_slice_wide_h = _widen8(b_slice_wide_b.w.hi, 0u); // high B to H
        DOT16_BLOCK_2()

        a_slice_1 = v_load_int16x2(a + k + 8);
        a_slice_2 = v_load_int16x2(a + k + 10);
        b_slice_wide_b = _widen4(b_slice_wide_n.w.hi, 0u); // high N to B
        b_slice_wide_h = _widen8(b_slice_wide_b.w.lo, 0u); // low B to H
        DOT16_BLOCK_2()

        a_slice_1 = v_load_int16x2(a + k + 12);
        a_slice_2 = v_load_int16x2(a + k + 14);
        b_slice_wide_h = _widen8(b_slice_wide_b.w.hi, 0u); // high B to H
        DOT16_BLOCK_2()
    }

    #undef DOT16_BLOCK_2

    size_t rem = (len - len_s16);
    if (rem > 0) {
        c += m_dotv_i16_i2_scalar_core(a + len_s16, b + (len_s16 >> 2), rem);
    }
    return c;
}

#ifdef SIMD_UNROLL
INLINE_OPTION
int32_t m_dotv_i8_i4(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 3; // unroll degree
    static const size_t deg = (2 + 1 + udeg); // +2 for bytes to words, +1 widen
    size_t tile = ((len >> deg) << deg);

    for (size_t k = 0; k < tile; k += (1 << deg)) {
        int8x4_t a_slice_1, a_slice_2;
        int4x8_t b_slice;
        int8x8_t b_slice_wide;

        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {                  // 0,  1,  2,  3
            b_slice = v_load_int4x8(b + ((k + i * 8) >> 1)); // 0,  4,  8, 12
            a_slice_1 = v_load_int8x4(a + k     + i * 8);    // 0,  8, 16, 24
            a_slice_2 = v_load_int8x4(a + k + 4 + i * 8);    // 4, 12, 20, 28
            b_slice_wide = _widen4(b_slice, 0u);
            asm volatile (
                "dot8 %[c], %[a1], %[bw_lo]\n\t"
                "dot8 %[c], %[a2], %[bw_hi]\n\t"
                : [c] "+r" (c)
                : [bw_lo] "r" (b_slice_wide.w.lo),
                  [bw_hi] "r" (b_slice_wide.w.hi),
                  [a1] "r" (a_slice_1),
                  [a2] "r" (a_slice_2)
            );
        }
    }

    // large tiles exhausted, finish with the regular SIMD core
    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i8_i4_simd_core(
            a + tile, b + (tile >> 1), rem
        );
    }
    return c;
}

#else
INLINE_OPTION
int32_t m_dotv_i8_i4(
    const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i8_i4_simd_core(a, b, len);
}

#endif // SIMD_UNROLL

static INLINE
int32_t m_dotv_i8_i4_simd_core(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s8 = ((len >> 3) << 3);
    for (size_t k = 0; k < len_s8; k += 8) {
        const int4x8_t b_slice = v_load_int4x8(b + (k >> 1));
        const int8x4_t a_slice_1 = v_load_int8x4(a + k);
        const int8x4_t a_slice_2 = v_load_int8x4(a + k + 4);
        const int8x8_t b_slice_wide = _widen4(b_slice, 0u);
        asm volatile (
            "dot8 %[c], %[a1], %[bw_lo]\n\t"
            "dot8 %[c], %[a2], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide.w.lo), [bw_hi] "r" (b_slice_wide.w.hi),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
        );
    }
    size_t rem = (len - len_s8);
    if (rem > 0) {
        c += m_dotv_i8_i4_scalar_core(a + len_s8, b + (len_s8 >> 1), rem);
    }
    return c;
}

#ifdef SIMD_UNROLL
INLINE_OPTION
int32_t m_dotv_i8_i2(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 2; // unroll degree
    static const size_t deg = (2 + 2 + udeg); // +2 for bytes to words, +2 widen
    size_t tile = ((len >> deg) << deg);

    for (size_t k = 0; k < tile; k += (1 << deg)) {
        int8x4_t a_slice_1, a_slice_2, a_slice_3, a_slice_4;
        int2x16_t b_slice;
        int4x16_t b_slice_wide_n;
        int8x8_t b_slice_wide_b;

        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {                    // 0,   1,  2
            b_slice = v_load_int2x16(b + ((k + i * 16) >> 2)); // 0,   4,  8
            a_slice_1 = v_load_int8x4(a + k      + i * 16);    // 0,  16, 32
            a_slice_2 = v_load_int8x4(a + k +  4 + i * 16);    // 4,  20, 36
            a_slice_3 = v_load_int8x4(a + k +  8 + i * 16);    // 8,  24, 40
            a_slice_4 = v_load_int8x4(a + k + 12 + i * 16);    // 12, 28, 44

            b_slice_wide_n = _widen2(b_slice, 0u); // C to N
            b_slice_wide_b = _widen4(b_slice_wide_n.w.lo, 0u); // low N to B
            asm volatile (
                "dot8 %[c], %[a1], %[bw_lo]\n\t"
                "dot8 %[c], %[a2], %[bw_hi]\n\t"
                : [c] "+r" (c)
                : [bw_lo] "r" (b_slice_wide_b.w.lo),
                  [bw_hi] "r" (b_slice_wide_b.w.hi),
                  [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
            );
            b_slice_wide_b = _widen4(b_slice_wide_n.w.hi, 0u); // high N to B
            asm volatile (
                "dot8 %[c], %[a3], %[bw_lo]\n\t"
                "dot8 %[c], %[a4], %[bw_hi]\n\t"
                : [c] "+r" (c)
                : [bw_lo] "r" (b_slice_wide_b.w.lo),
                  [bw_hi] "r" (b_slice_wide_b.w.hi),
                  [a3] "r" (a_slice_3), [a4] "r" (a_slice_4)
            );
        }
    }

    // large tiles exhausted, finish with the regular SIMD core
    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i8_i2_simd_core(
            a + tile, b + (tile >> 2), rem
        );
    }
    return c;
}

#else
INLINE_OPTION
int32_t m_dotv_i8_i2(
    const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i8_i2_simd_core(a, b, len);
}

#endif // SIMD_UNROLL

static INLINE
int32_t m_dotv_i8_i2_simd_core(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s16 = ((len >> 4) << 4);
    for (size_t k = 0; k < len_s16; k += 16) {
        const int2x16_t b_slice = v_load_int2x16(b + (k >> 2));
        const int8x4_t a_slice_1 = v_load_int8x4(a + k);
        const int8x4_t a_slice_2 = v_load_int8x4(a + k + 4);
        const int8x4_t a_slice_3 = v_load_int8x4(a + k + 8);
        const int8x4_t a_slice_4 = v_load_int8x4(a + k + 12);
        int4x16_t b_slice_wide_n;
        int8x8_t b_slice_wide_b;

        b_slice_wide_n = _widen2(b_slice, 0u); // C to N
        b_slice_wide_b = _widen4(b_slice_wide_n.w.lo, 0u); // low N to B
        asm volatile (
            "dot8 %[c], %[a1], %[bw_lo]\n\t"
            "dot8 %[c], %[a2], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide_b.w.lo),
              [bw_hi] "r" (b_slice_wide_b.w.hi),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
        );
        b_slice_wide_b = _widen4(b_slice_wide_n.w.hi, 0u); // high N to B
        asm volatile (
            "dot8 %[c], %[a3], %[bw_lo]\n\t"
            "dot8 %[c], %[a4], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide_b.w.lo),
              [bw_hi] "r" (b_slice_wide_b.w.hi),
              [a3] "r" (a_slice_3), [a4] "r" (a_slice_4)
        );
    }
    size_t rem = (len - len_s16);
    if (rem > 0) {
        c += m_dotv_i8_i2_scalar_core(a + len_s16, b + (len_s16 >> 2), rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i4_i2(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s16 = ((len >> 4) << 4);
    for (size_t k = 0; k < len_s16; k += 16) {
        const int2x16_t b_slice = v_load_int2x16(b + (k >> 2));
        const int4x8_t a_slice_1 = v_load_int4x8(a + (k >> 1));
        const int4x8_t a_slice_2 = v_load_int4x8(a + (k >> 1) + 4);
        const int4x16_t b_slice_wide = _widen2(b_slice, 0u); // C to N
        asm volatile (
            "dot4 %[c], %[a1], %[bw_lo]\n\t"
            "dot4 %[c], %[a2], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide.w.lo),
              [bw_hi] "r" (b_slice_wide.w.hi),
              [a1] "r" (a_slice_1),
              [a2] "r" (a_slice_2)
        );
    }
    size_t rem = (len - len_s16);
    if (rem > 0) {
        c += m_dotv_i4_i2_scalar_core(
            a + (len_s16 >> 1), b + (len_s16 >> 2), rem
        );
    }
    return c;
}

// -----------------------------------------------------------------------------
// SIMD data formatting functions
// -----------------------------------------------------------------------------

void m_txp_2x2_i16(
    const size_t b_cols,
    const int16_t b[][b_cols], // pointer to an array of b_cols el, (*b)[b_cols]
    const size_t k, const size_t n,
    int16x4_t* bs_t16)
{
    // b_cols = row stride (N), k = K index, n = N index
    const int16x2_t bs_0 = v_load_int16x2(&b[(k<<1) + 0][n<<1]);
    const int16x2_t bs_1 = v_load_int16x2(&b[(k<<1) + 1][n<<1]);
    // b transpose
    *bs_t16 = _txp16(bs_0, bs_1);
}

void m_txp_4x4_i8(
    const size_t b_cols,
    const int8_t b[][b_cols],
    const size_t k, const size_t n,
    int8x8_t* bs_t16_02, int8x8_t* bs_t16_13)
{
    // b_cols = row stride (N), k = K index, n = N index
    const int8x4_t bs_0 = v_load_int8x4(&b[(k<<2) + 0][n<<2]);
    const int8x4_t bs_1 = v_load_int8x4(&b[(k<<2) + 1][n<<2]);
    const int8x4_t bs_2 = v_load_int8x4(&b[(k<<2) + 2][n<<2]);
    const int8x4_t bs_3 = v_load_int8x4(&b[(k<<2) + 3][n<<2]);

    // b transpose
    int8x8_t bs_t8_01, bs_t8_23;
    asm volatile("txp8 %0, %1, %2" : "=r"(bs_t8_01) : "r"(bs_0), "r"(bs_1));
    asm volatile("txp8 %0, %1, %2" : "=r"(bs_t8_23) : "r"(bs_2), "r"(bs_3));
    asm volatile(
        "txp16 %0, %1, %2"
        : "=r"(*bs_t16_02)
        : "r"(bs_t8_01.w.lo), "r"(bs_t8_23.w.lo)
    );
    asm volatile(
        "txp16 %0, %1, %2"
        : "=r"(*bs_t16_13)
        : "r"(bs_t8_01.w.hi), "r"(bs_t8_23.w.hi)
    );

    /*
    // wrappers require quite a bit of casting, and have worse scheduling
    // b transpose
    int8x8_t bs_t8_01, bs_t8_23;
    bs_t8_01 = _txp8(bs_0, bs_1);
    bs_t8_23 = _txp8(bs_2, bs_3);

    // type casts for txp
    int16x2_t bs_t8_01_tc, bs_t8_23_tc;
    int16x4_t bs_t16_tc;

    bs_t8_01_tc.v = (int32_t)bs_t8_01.w.lo.v;
    bs_t8_23_tc.v = (int32_t)bs_t8_23.w.lo.v;
    bs_t16_tc = _txp16(bs_t8_01_tc, bs_t8_23_tc);
    bs_t16_02->d = bs_t16_tc.d;

    bs_t8_01_tc.v = (int32_t)bs_t8_01.w.hi.v;
    bs_t8_23_tc.v = (int32_t)bs_t8_23.w.hi.v;
    bs_t16_tc = _txp16(bs_t8_01_tc, bs_t8_23_tc);
    bs_t16_13->d = bs_t16_tc.d;
    */
}

#endif

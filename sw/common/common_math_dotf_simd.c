#include "common_math.h"
#include "common_math_simd_intrinsics.h"
#include "common_math_simd_v_load_store.h"

#ifdef __riscv_xsimd

INLINE
void m_dotf_i8_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    #define K_ATOMIC 4 // one dot8 retires 4 logical elements
    #define K_UNROLL 4
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // 8 contiguous bytes of one 'a' row, across 2 regs
    #define LOAD_A_ROW_2(r, off) \
        a_arr[r][0] = v_load_int8x4(a + (r)*lda + kk + (off)); \
        a_arr[r][1] = v_load_int8x4(a + (r)*lda + kk + (off) + 4)

    // 4 dot8 into 2 accumulators
    #define DOT8_BLOCK_4(c0, c1, x0, x1, a00, a01, a10, a11) \
        asm volatile ( \
            "dot8 %[_c0], %[_x1], %[_a01]\n\t" \
            "dot8 %[_c1], %[_x0], %[_a10]\n\t" \
            "dot8 %[_c1], %[_x1], %[_a11]\n\t" \
            "dot8 %[_c0], %[_x0], %[_a00]\n\t" \
            : [_c0] "+r" (c0), [_c1] "+r" (c1) \
            : [_x0] "r" (x0), [_x1] "r" (x1), \
              [_a00] "r" (a00), [_a01] "r" (a01), \
              [_a10] "r" (a10), [_a11] "r" (a11) \
            : \
        )

    int32_t c[4] = {0};
    const size_t kms = ((k / K_STEP) * K_STEP); // k max step-aligned

    for (size_t kk = 0; kk < kms; kk += K_STEP) {
        int8x4_t x_arr[2], a_arr[4][2];

        x_arr[0] = v_load_int8x4(x + kk + 0);
        x_arr[1] = v_load_int8x4(x + kk + 4);
        LOAD_A_ROW_2(0, 0);
        LOAD_A_ROW_2(1, 0);
        LOAD_A_ROW_2(2, 0);
        LOAD_A_ROW_2(3, 0);
        DOT8_BLOCK_4(
            c[0], c[1],
            x_arr[0], x_arr[1],
            a_arr[0][0], a_arr[0][1], a_arr[1][0], a_arr[1][1]
        );
        DOT8_BLOCK_4(
            c[2], c[3],
            x_arr[0], x_arr[1],
            a_arr[2][0], a_arr[2][1], a_arr[3][0], a_arr[3][1]
        );

        x_arr[0] = v_load_int8x4(x + kk + 8);
        x_arr[1] = v_load_int8x4(x + kk + 12);
        LOAD_A_ROW_2(0, 8);
        LOAD_A_ROW_2(1, 8);
        LOAD_A_ROW_2(2, 8);
        LOAD_A_ROW_2(3, 8);
        DOT8_BLOCK_4(
            c[0], c[1],
            x_arr[0], x_arr[1],
            a_arr[0][0], a_arr[0][1], a_arr[1][0], a_arr[1][1]
        );
        DOT8_BLOCK_4(
            c[2], c[3],
            x_arr[0], x_arr[1],
            a_arr[2][0], a_arr[2][1], a_arr[3][0], a_arr[3][1]
        );
    }

    // k tail: reuse dotv, which has its own tail
    if (kms < k) {
        for (size_t i = 0; i < 4; i++) {
            c[i] += m_dotv_i8_i8(a + i*lda + kms, x + kms, k - kms);
        }
    }

    y[0] = c[0];
    y[1] = c[1];
    y[2] = c[2];
    y[3] = c[3];

    #undef DOT8_BLOCK_4
    #undef LOAD_A_ROW_2
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

INLINE
void m_dotf_i4_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    #define K_ATOMIC 8 // one int4 word: 8 nibbles -> 2 dot8 operands
    #define K_UNROLL 2
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // 'a' is walked with its own pointer instead of being indexed off kk
    #define A_ROW_ADDR(r, off) (ap + (r)*lda_b + ((off) >> 1))

    // 4 dot8 into 2 accumulators, one row each
    #define DOT8_BLOCK_4(c0, c1, x0, x1, wa0, wa1) \
        asm volatile ( \
            "dot8 %[_c0], %[_x0], %[_a0lo]\n\t" \
            "dot8 %[_c0], %[_x1], %[_a0hi]\n\t" \
            "dot8 %[_c1], %[_x0], %[_a1lo]\n\t" \
            "dot8 %[_c1], %[_x1], %[_a1hi]\n\t" \
            : [_c0] "+r" (c0), [_c1] "+r" (c1) \
            : [_x0] "r" (x0), [_x1] "r" (x1), \
              [_a0lo] "r" ((wa0).w.lo), [_a0hi] "r" ((wa0).w.hi), \
              [_a1lo] "r" ((wa1).w.lo), [_a1hi] "r" ((wa1).w.hi) \
            : \
        )

    // rows are taken a pair at a time
    #define DOT8_ROW_I4_PAIR(r0, r1, off) \
        do { \
            const int4x8_t _a0 = v_load_int4x8(A_ROW_ADDR(r0, off)); \
            const int4x8_t _a1 = v_load_int4x8(A_ROW_ADDR(r1, off)); \
            const int8x8_t _wa0 = _widen4(_a0, 0u); \
            const int8x8_t _wa1 = _widen4(_a1, 0u); \
            DOT8_BLOCK_4(c[r0], c[r1], x0, x1, _wa0, _wa1); \
        } while (0)

    int32_t c[4] = {0};
    const size_t kms = ((k / K_STEP) * K_STEP);
    const size_t lda_b = (lda >> 1); // row stride of 'a', in bytes
    const int8_t* ap = a;

    for (size_t kk = 0; kk < kms; kk += K_STEP, ap += (K_STEP >> 1)) {
        int8x4_t x0, x1;

        x0 = v_load_int8x4(x + kk + 0);
        x1 = v_load_int8x4(x + kk + 4);
        DOT8_ROW_I4_PAIR(0, 1, 0);
        DOT8_ROW_I4_PAIR(2, 3, 0);

        x0 = v_load_int8x4(x + kk + 8);
        x1 = v_load_int8x4(x + kk + 12);
        DOT8_ROW_I4_PAIR(0, 1, 8);
        DOT8_ROW_I4_PAIR(2, 3, 8);
    }

    // k tail: reuse dotv, which has its own tail
    // note the operand swap - dotv names the wide type first
    if (kms < k) {
        for (size_t i = 0; i < 4; i++) {
            c[i] += m_dotv_i8_i4(
                x + kms, a + i*(lda >> 1) + (kms >> 1), k - kms
            );
        }
    }

    y[0] = c[0];
    y[1] = c[1];
    y[2] = c[2];
    y[3] = c[3];

    #undef DOT8_ROW_I4_PAIR
    #undef DOT8_BLOCK_4
    #undef A_ROW_ADDR
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

INLINE
void m_dotf_i2_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    #define K_ATOMIC 16 // one int2 word: 16 crumbs -> 4 dot8 operands
    #define K_UNROLL 1
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // 4 dot8 into one accumulator, all from a single int2 word of row r
    #define DOT8_BLOCK_4(cr, x0, x1, x2, x3, araw) \
        do { \
            const int4x16_t _n = _widen2(araw, 0u); \
            const int8x8_t _b_lo = _widen4(_n.w.lo, 0u); \
            const int8x8_t _b_hi = _widen4(_n.w.hi, 0u); \
            asm volatile ( \
                "dot8 %[_c], %[_x0], %[_b0]\n\t" \
                "dot8 %[_c], %[_x1], %[_b1]\n\t" \
                "dot8 %[_c], %[_x2], %[_b2]\n\t" \
                "dot8 %[_c], %[_x3], %[_b3]\n\t" \
                : [_c] "+r" (cr) \
                : [_x0] "r" (x0), [_x1] "r" (x1), \
                  [_x2] "r" (x2), [_x3] "r" (x3), \
                  [_b0] "r" (_b_lo.w.lo), [_b1] "r" (_b_lo.w.hi), \
                  [_b2] "r" (_b_hi.w.lo), [_b3] "r" (_b_hi.w.hi) \
                : \
            ); \
        } while (0)

    // load _a1 before widening _a0 to work around 1 clk load-to-use delay
    #define DOT8_ROW_I2_PAIR(r0, r1) \
        do { \
            const int2x16_t _a0 = v_load_int2x16(ap + (r0)*lda_b); \
            const int2x16_t _a1 = v_load_int2x16(ap + (r1)*lda_b); \
            DOT8_BLOCK_4(c[r0], x0, x1, x2, x3, _a0); \
            DOT8_BLOCK_4(c[r1], x0, x1, x2, x3, _a1); \
        } while (0)

    int32_t c[4] = {0};
    const size_t kms = ((k / K_STEP) * K_STEP);
    const size_t lda_b = (lda >> 2); // row stride of 'a', in bytes
    const int8_t* ap = a;

    for (size_t kk = 0; kk < kms; kk += K_STEP, ap += (K_STEP >> 2)) {
        int8x4_t x0, x1, x2, x3;

        x0 = v_load_int8x4(x + kk + 0);
        x1 = v_load_int8x4(x + kk + 4);
        x2 = v_load_int8x4(x + kk + 8);
        x3 = v_load_int8x4(x + kk + 12);
        DOT8_ROW_I2_PAIR(0, 1);
        DOT8_ROW_I2_PAIR(2, 3);
    }

    // k tail: reuse dotv, which has its own tail
    // note the operand swap - dotv names the wide type first
    if (kms < k) {
        for (size_t i = 0; i < 4; i++) {
            c[i] += m_dotv_i8_i2(
                x + kms, a + i*(lda >> 2) + (kms >> 2), k - kms
            );
        }
    }

    y[0] = c[0];
    y[1] = c[1];
    y[2] = c[2];
    y[3] = c[3];

    #undef DOT8_ROW_I2_PAIR
    #undef DOT8_BLOCK_4
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

#endif // __riscv_xsimd

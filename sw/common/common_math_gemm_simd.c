#include "common_math.h"
#include "common_math_simd_intrinsics.h"
#include "common_math_simd_v_load_store.h"

#ifdef __riscv_xsimd

// gemm microkernel handling (MR x NR) output tile (K tail included)
// caller is responsible for M % MR, and N % NR

// doesn't change for any kernel, same transpose rule, same local variables
#define STORE_C \
    if (c_t) { \
        /* flips the C dims, i.e. transpose on the fly */ \
        c[0*ldc+0] = co[0][0]; \
        c[0*ldc+1] = co[1][0]; \
        c[0*ldc+2] = co[2][0]; \
        c[0*ldc+3] = co[3][0]; \
        c[1*ldc+0] = co[0][1]; \
        c[1*ldc+1] = co[1][1]; \
        c[1*ldc+2] = co[2][1]; \
        c[1*ldc+3] = co[3][1]; \
    } else { \
        c[0*ldc] = co[0][0]; \
        c[1*ldc] = co[1][0]; \
        c[2*ldc] = co[2][0]; \
        c[3*ldc] = co[3][0]; \
        c[0*ldc+1] = co[0][1]; \
        c[1*ldc+1] = co[1][1]; \
        c[2*ldc+1] = co[2][1]; \
        c[3*ldc+1] = co[3][1]; \
    }

INLINE
void m_gemm_ukr_i8_i8_mr4x2(
    const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t)
{
    #define NR 2
    #define K_ATOMIC 4 // one dot8 retires 4 logical elements
    #define K_UNROLL 4
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    #define LOAD_MR_X_NR(off) \
        a_arr[0] = v_load_int8x4(a + (0)*lda + kk + (off)); \
        a_arr[1] = v_load_int8x4(a + (1)*lda + kk + (off)); \
        a_arr[2] = v_load_int8x4(a + (2)*lda + kk + (off)); \
        a_arr[3] = v_load_int8x4(a + (3)*lda + kk + (off)); \
        b_arr[0] = v_load_int8x4(b + (0)*ldb + kk + (off)); \
        b_arr[1] = v_load_int8x4(b + (1)*ldb + kk + (off));

    #define DOT8_BLOCK_8 \
        asm volatile ( \
            "dot8 %[_c10], %[_a1], %[_b0]\n\t" \
            "dot8 %[_c20], %[_a2], %[_b0]\n\t" \
            "dot8 %[_c30], %[_a3], %[_b0]\n\t" \
            "dot8 %[_c11], %[_a1], %[_b1]\n\t" \
            "dot8 %[_c21], %[_a2], %[_b1]\n\t" \
            "dot8 %[_c31], %[_a3], %[_b1]\n\t" \
            "dot8 %[_c00], %[_a0], %[_b0]\n\t" \
            "dot8 %[_c01], %[_a0], %[_b1]\n\t" \
            : [_c00] "+r" (co[0][0]), [_c01] "+r" (co[0][1]), \
              [_c10] "+r" (co[1][0]), [_c11] "+r" (co[1][1]), \
              [_c20] "+r" (co[2][0]), [_c21] "+r" (co[2][1]), \
              [_c30] "+r" (co[3][0]), [_c31] "+r" (co[3][1]) \
            : [_a0] "r" (a_arr[0]), [_a1] "r" (a_arr[1]), \
              [_a2] "r" (a_arr[2]), [_a3] "r" (a_arr[3]), \
              [_b0] "r" (b_arr[0]), [_b1] "r" (b_arr[1]) \
            : \
        )

    int32_t co[4][2] = {0};
    const size_t kms = ((k / K_STEP) * K_STEP); // k max step-aligned

    for (size_t kk = 0; kk < kms; kk += K_STEP) {
        int8x4_t a_arr[4], b_arr[2];
        LOAD_MR_X_NR(0)
        DOT8_BLOCK_8;
        LOAD_MR_X_NR(4)
        DOT8_BLOCK_8;
        LOAD_MR_X_NR(8)
        DOT8_BLOCK_8;
        LOAD_MR_X_NR(12)
        DOT8_BLOCK_8;
    }

    // k tail: reuse dotv, which has its own tail
    if (kms < k) {
        for (size_t i = 0; i < NR; i++) {
            const int8_t* b_ptr = (b + i*ldb + kms);
            co[0][i] += m_dotv_i8_i8((a + 0*lda + kms), b_ptr, (k - kms));
            co[1][i] += m_dotv_i8_i8((a + 1*lda + kms), b_ptr, (k - kms));
            co[2][i] += m_dotv_i8_i8((a + 2*lda + kms), b_ptr, (k - kms));
            co[3][i] += m_dotv_i8_i8((a + 3*lda + kms), b_ptr, (k - kms));
        }
    }
    // store back
    STORE_C

    #undef NR
    #undef DOT8_BLOCK_8
    #undef LOAD_MR_X_NR
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

INLINE
void m_gemm_ukr_i4_i8_mr4x2(
    const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t)
{
    #define NR 2
    #define K_ATOMIC 8 // one int4 word: 8 nibbles -> 2 dot8 operands
    #define K_UNROLL 2
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // both k-halves of 'b' for both n rows,
    // one k-atomic's worth of 'b' is loaded once and reused by every row pair
    #define LOAD_B4(off) \
        b_arr[0] = v_load_int8x4(b + (0)*ldb + kk + (off)); \
        b_arr[1] = v_load_int8x4(b + (1)*ldb + kk + (off)); \
        b_arr[2] = v_load_int8x4(b + (0)*ldb + kk + (off) + 4); \
        b_arr[3] = v_load_int8x4(b + (1)*ldb + kk + (off) + 4);

    // 8 dots, 4 accumulators: 2 'a' rows x 2 'b' rows, both k-halves
    #define DOT8_BLOCK_8(c0n0, c0n1, c1n0, c1n1, wa0, wa1) \
        asm volatile ( \
            "dot8 %[_c0], %[_a0lo], %[_b0]\n\t" \
            "dot8 %[_c1], %[_a0lo], %[_b1]\n\t" \
            "dot8 %[_c0], %[_a0hi], %[_b2]\n\t" \
            "dot8 %[_c1], %[_a0hi], %[_b3]\n\t" \
            "dot8 %[_c2], %[_a1lo], %[_b0]\n\t" \
            "dot8 %[_c3], %[_a1lo], %[_b1]\n\t" \
            "dot8 %[_c2], %[_a1hi], %[_b2]\n\t" \
            "dot8 %[_c3], %[_a1hi], %[_b3]\n\t" \
            : [_c0] "+r" (c0n0), [_c1] "+r" (c0n1), \
              [_c2] "+r" (c1n0), [_c3] "+r" (c1n1) \
            : [_a0lo] "r" ((wa0).w.lo), [_a0hi] "r" ((wa0).w.hi), \
              [_a1lo] "r" ((wa1).w.lo), [_a1hi] "r" ((wa1).w.hi), \
              [_b0] "r" (b_arr[0]), [_b1] "r" (b_arr[1]), \
              [_b2] "r" (b_arr[2]), [_b3] "r" (b_arr[3]) \
            : \
        )

    // A rows are taken a PAIR at a time
    #define DOT8_ROW_I4_PAIR(r0, r1, off) \
        do { \
            const int4x8_t _a0 = v_load_int4x8(ap + (r0)*lda_b + ((off) >> 1));\
            const int4x8_t _a1 = v_load_int4x8(ap + (r1)*lda_b + ((off) >> 1));\
            const int8x8_t _wa0 = _widen4(_a0, 0u); \
            const int8x8_t _wa1 = _widen4(_a1, 0u); \
            DOT8_BLOCK_8( \
                co[r0][0], co[r0][1], co[r1][0], co[r1][1], _wa0, _wa1 \
            ); \
        } while (0)

    int32_t co[4][2] = {0};
    const size_t kms = ((k / K_STEP) * K_STEP); // k max step-aligned
    const size_t lda_b = (lda >> 1); // row stride of 'a', in bytes
    const int8_t* ap = a;

    for (size_t kk = 0; kk < kms; kk += K_STEP, ap += (K_STEP >> 1)) {
        int8x4_t b_arr[4];

        LOAD_B4(0)
        DOT8_ROW_I4_PAIR(0, 1, 0);
        DOT8_ROW_I4_PAIR(2, 3, 0);

        LOAD_B4(8)
        DOT8_ROW_I4_PAIR(0, 1, 8);
        DOT8_ROW_I4_PAIR(2, 3, 8);
    }

    // k tail: reuse dotv, which has its own tail
    // note the operand swap - dotv names the wide type first
    if (kms < k) {
        for (size_t i = 0; i < NR; i++) {
            const int8_t* b_ptr = (b + i*ldb + kms);
            const uint32_t lda_b = (lda >> 1);
            const uint32_t kms_b = (kms >> 1);
            co[0][i] += m_dotv_i8_i4(b_ptr, (a + 0*lda_b + kms_b), (k - kms));
            co[1][i] += m_dotv_i8_i4(b_ptr, (a + 1*lda_b + kms_b), (k - kms));
            co[2][i] += m_dotv_i8_i4(b_ptr, (a + 2*lda_b + kms_b), (k - kms));
            co[3][i] += m_dotv_i8_i4(b_ptr, (a + 3*lda_b + kms_b), (k - kms));
        }
    }
    // store back
    STORE_C

    #undef NR
    #undef DOT8_ROW_I4_PAIR
    #undef LOAD_B4
    #undef DOT8_BLOCK_8
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

INLINE
void m_gemm_ukr_i2_i8_mr4x2(
    const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t)
{
    #define NR 2
    #define K_ATOMIC 16 // one int2 word: 16 crumbs -> 4 dot8 operands
    #define K_UNROLL 1
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // entire MR x NR tile of B, peak reg file pressure
    #define LOAD_B8 \
        b_arr[0] = v_load_int8x4(b + (0)*ldb + kk + 0); \
        b_arr[1] = v_load_int8x4(b + (0)*ldb + kk + 4); \
        b_arr[2] = v_load_int8x4(b + (0)*ldb + kk + 8); \
        b_arr[3] = v_load_int8x4(b + (0)*ldb + kk + 12); \
        b_arr[4] = v_load_int8x4(b + (1)*ldb + kk + 0); \
        b_arr[5] = v_load_int8x4(b + (1)*ldb + kk + 4); \
        b_arr[6] = v_load_int8x4(b + (1)*ldb + kk + 8); \
        b_arr[7] = v_load_int8x4(b + (1)*ldb + kk + 12);

    // 8 dots: one 'A' row, widened to 4 int8 words
    #define DOT8_BLOCK_8(cn0, cn1, araw) \
        do { \
            const int4x16_t _n = _widen2(araw, 0u); \
            const int8x8_t _wlo = _widen4(_n.w.lo, 0u); \
            const int8x8_t _whi = _widen4(_n.w.hi, 0u); \
            asm volatile ( \
                "dot8 %[_cn0], %[_b00], %[_a0]\n\t" \
                "dot8 %[_cn1], %[_b10], %[_a0]\n\t" \
                "dot8 %[_cn0], %[_b01], %[_a1]\n\t" \
                "dot8 %[_cn1], %[_b11], %[_a1]\n\t" \
                "dot8 %[_cn0], %[_b02], %[_a2]\n\t" \
                "dot8 %[_cn1], %[_b12], %[_a2]\n\t" \
                "dot8 %[_cn0], %[_b03], %[_a3]\n\t" \
                "dot8 %[_cn1], %[_b13], %[_a3]\n\t" \
                : [_cn0] "+r" (cn0), [_cn1] "+r" (cn1) \
                : [_a0] "r" (_wlo.w.lo), [_a1] "r" (_wlo.w.hi), \
                  [_a2] "r" (_whi.w.lo), [_a3] "r" (_whi.w.hi), \
                  [_b00] "r" (b_arr[0]), [_b01] "r" (b_arr[1]), \
                  [_b02] "r" (b_arr[2]), [_b03] "r" (b_arr[3]), \
                  [_b10] "r" (b_arr[4]), [_b11] "r" (b_arr[5]), \
                  [_b12] "r" (b_arr[6]), [_b13] "r" (b_arr[7]) \
                : \
            ); \
        } while (0)

    // both raw words loaded before either is widened, to cover load-to-use
    #define DOT8_ROW_I2_PAIR(r0, r1) \
        do { \
            const int2x16_t _a0 = v_load_int2x16(ap + (r0)*lda_b); \
            const int2x16_t _a1 = v_load_int2x16(ap + (r1)*lda_b); \
            DOT8_BLOCK_8(co[r0][0], co[r0][1], _a0); \
            DOT8_BLOCK_8(co[r1][0], co[r1][1], _a1); \
        } while (0)

    int32_t co[4][2] = {0};
    const size_t kms = ((k / K_STEP) * K_STEP); // k max step-aligned
    const size_t lda_b = (lda >> 2); // row stride of 'a', in bytes
    const int8_t* ap = a;

    for (size_t kk = 0; kk < kms; kk += K_STEP, ap += (K_STEP >> 2)) {
        int8x4_t b_arr[8];

        LOAD_B8
        DOT8_ROW_I2_PAIR(0, 1);
        DOT8_ROW_I2_PAIR(2, 3);
    }

    // k tail: reuse dotv, which has its own tail
    // note the operand swap - dotv names the wide type first
    if (kms < k) {
        for (size_t i = 0; i < NR; i++) {
            const int8_t* b_ptr = (b + i*ldb + kms);
            const size_t kms_b = (kms >> 2);
            co[0][i] += m_dotv_i8_i2(b_ptr, (a + 0*lda_b + kms_b), (k - kms));
            co[1][i] += m_dotv_i8_i2(b_ptr, (a + 1*lda_b + kms_b), (k - kms));
            co[2][i] += m_dotv_i8_i2(b_ptr, (a + 2*lda_b + kms_b), (k - kms));
            co[3][i] += m_dotv_i8_i2(b_ptr, (a + 3*lda_b + kms_b), (k - kms));
        }
    }
    // store back
    STORE_C

    #undef NR
    #undef DOT8_ROW_I2_PAIR
    #undef LOAD_B8
    #undef DOT8_BLOCK_8
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

#undef STORE_C

#endif // __riscv_xsimd

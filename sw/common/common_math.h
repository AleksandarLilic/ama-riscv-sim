#ifndef COMMON_MATH_H
#define COMMON_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#ifdef PARTIAL_ZBB_SUPPORT
static INLINE int32_t max(int32_t a, int32_t b) {
    int32_t c;
    asm volatile(
        ".insn r 0x33, 0x6, 0x5, %[out], %[in0], %[in1]"
        : [out] "=r" (c)
        : [in0] "r" (a), [in1] "r" (b)
    );
    return c;
}

static INLINE uint32_t maxu(uint32_t a, uint32_t b) {
    uint32_t c;
    asm volatile(
        ".insn r 0x33, 0x7, 0x5, %[out], %[in0], %[in1]"
        : [out] "=r" (c)
        : [in0] "r" (a), [in1] "r" (b)
    );
    return c;
}

static INLINE int32_t min(int32_t a, int32_t b) {
    int32_t c;
    asm volatile(
        ".insn r 0x33, 0x4, 0x5, %[out], %[in0], %[in1]"
        : [out] "=r" (c)
        : [in0] "r" (a), [in1] "r" (b)
    );
    return c;
}

static INLINE uint32_t minu(uint32_t a, uint32_t b) {
    uint32_t c;
    asm volatile(
        ".insn r 0x33, 0x5, 0x5, %[out], %[in0], %[in1]"
        : [out] "=r" (c)
        : [in0] "r" (a), [in1] "r" (b)
    );
    return c;
}

#else
static INLINE int32_t max(int32_t a, int32_t b) {
    return a > b ? a : b;
}

static INLINE uint32_t maxu(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

static INLINE int32_t min(int32_t a, int32_t b) {
    return a < b ? a : b;
}

static INLINE uint32_t minu(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}
#endif

// -----------------------------------------------------------------------------
// arithv
// -----------------------------------------------------------------------------

// add & sub
void m_add_i16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len);
void m_add_i8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len);
void m_sub_i16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len);
void m_sub_i8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len);

// mul and mul unsigned
void m_mul_i16(
    const int16_t* a, const int16_t* b, int32_t* c, const size_t len);
void m_mul_i8(
    const int8_t* a, const int8_t* b, int16_t* c, const size_t len);
void m_mul_u16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len);
void m_mul_u8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len);

// -----------------------------------------------------------------------------
// level-1v: dot products
// -----------------------------------------------------------------------------
// m_dotv_<ta>_<tb> computes dot product of two vectors:
//   y = sum_{j<k} a[j] * b[j]
//
// the type suffixes (wide, narrow)
// -----------------------------------------------------------------------------

// dot product
int32_t m_dotv_i16_i16(
    const int16_t* a, const int16_t* b, const size_t len);
int32_t m_dotv_i8_i8(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i4_i4(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i2_i2(
    const int8_t* a, const int8_t* b, const size_t len);

// dot product w/ unpacking
int32_t m_dotv_i16_i8(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i16_i4(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i16_i2(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i8_i4(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i8_i2(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i4_i2(
    const int8_t* a, const int8_t* b, const size_t len);

#ifdef __riscv_xsimd
// TODO: add scalar versions, currently only supported by SIMD
// and also to move to dedicated .h/.c

void m_txp_2x2_i16(
    const size_t b_cols,
    const int16_t b[][b_cols], // pointer to an array of b_cols el, (*b)[b_cols]
    const size_t k, const size_t n,
    int16x4_t* bs_t16);

void m_txp_4x4_i8(
    const size_t b_cols,
    const int8_t b[][b_cols],
    const size_t k, const size_t n,
    int8x8_t* bs_t16_02, int8x8_t* bs_t16_13);

#endif

// -----------------------------------------------------------------------------
// level-1f: fused dot products
// -----------------------------------------------------------------------------
// m_dotf_<ta>_<tx>_mr<MR> computes MR dot products sharing one x vector:
//
//   y[i] = sum_{j<k} a[i*lda + j] * x[j],  i in [0, MR)
//
// the type suffixes are (matrix, vector),
// which is the reverse of dotv's (wide, narrow):
// the same operand pair is m_dotv_i8_i4 but m_dotf_i4_i8
//
// 'a' is the MR-row matrix and is loaded MR times per step;
// 'x' is the shared vector and is loaded once;
// 'lda' is in logical elements, not storage bytes, for every type;
//
// kernel-private K info:
//   K_ATOMIC  smallest number of logical k elements consumable without a
//             partial-word access:
//             dot width (SIMD), load slice (LOAD_OPT), packing of 'a' (scalar)
//   K_UNROLL  K_ATOMICs per inner iteration
//   K_STEP    K_ATOMIC * K_UNROLL, the inner-loop stride
//
// -----------------------------------------------------------------------------

void m_dotf_i8_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y);
void m_dotf_i4_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y);
void m_dotf_i2_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y);

// MR selection
//
// note the +-2048 lw immediate max when raising it:

// at MR=4, lda=256 the largest row offset is 780 for i8 (256*3+12),
// but MR=8, lda=512 is 3596, and would fall back to extra pointer bumps
// calc: (lda*(MR-1) + 4*(UNROLL-1))

#define M_DOTF_I8_I8_MR 4
#define M_DOTF_I4_I8_MR 4
#define M_DOTF_I2_I8_MR 4

#define M_CONCAT(a, b) a##b
#define M_EXPAND_CONCAT(a, b)  M_CONCAT(a, b)
#define M_DOTF_I8_I8_KER M_EXPAND_CONCAT(m_dotf_i8_i8_mr, M_DOTF_I8_I8_MR)
#define M_DOTF_I4_I8_KER M_EXPAND_CONCAT(m_dotf_i4_i8_mr, M_DOTF_I4_I8_MR)
#define M_DOTF_I2_I8_KER M_EXPAND_CONCAT(m_dotf_i2_i8_mr, M_DOTF_I2_I8_MR)

_Static_assert(
    M_DOTF_I8_I8_MR == 4, "M_DOTF_I8_I8_MR: no kernel implemented for this MR"
);
_Static_assert(
    M_DOTF_I4_I8_MR == 4, "M_DOTF_I4_I8_MR: no kernel implemented for this MR"
);
_Static_assert(
    M_DOTF_I2_I8_MR == 4, "M_DOTF_I2_I8_MR: no kernel implemented for this MR"
);

// -----------------------------------------------------------------------------
// level-2: matrix-vector product
// -----------------------------------------------------------------------------
// m_gemv_<ta>_<tx> computes m dot products sharing one x vector:
//
//   y[i] = sum_{j<k} a[i*lda + j] * x[j],  i in [0, m)
//
// plain y = A*x: row-major 'a', no transpose, and no alpha/beta, so 'y' is
// write-only - the caller never pre-zeroes it and the kernel never reads it back
//
// type suffixes and 'lda' are level-1f's, above
//
// where the dimensions are owned:
//   the kernel owns the ones it chose      -> k % K_STEP, so k is passed
//                                             through untouched
//   gemv owns the one the caller chose     -> m % MR, blocked by MR and then
//                                             mopped up one row at a time
// -----------------------------------------------------------------------------

void m_gemv_i8_i8(
    const size_t m, const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y);
void m_gemv_i4_i8(
    const size_t m, const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y);
void m_gemv_i2_i8(
    const size_t m, const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y);

// -----------------------------------------------------------------------------
// level-3: matrix-matrix product ukr
// -----------------------------------------------------------------------------

void m_gemm_ukr_i8_i8_mr4x2(
    const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t);

void m_gemm_ukr_i4_i8_mr4x2(
    const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t);

void m_gemm_ukr_i2_i8_mr4x2(
    const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t);

#define M_GEMM_I8_I8_MR 4
#define M_GEMM_I8_I8_NR 2
#define M_GEMM_I4_I8_MR 4
#define M_GEMM_I4_I8_NR 2
#define M_GEMM_I2_I8_MR 4
#define M_GEMM_I2_I8_NR 2

#define M_GEMM_I8_I8_KER \
    M_EXPAND_CONCAT(M_EXPAND_CONCAT(m_gemm_ukr_i8_i8_mr, M_GEMM_I8_I8_MR), \
                    M_EXPAND_CONCAT(x, M_GEMM_I8_I8_NR))

#define M_GEMM_I4_I8_KER \
    M_EXPAND_CONCAT(M_EXPAND_CONCAT(m_gemm_ukr_i4_i8_mr, M_GEMM_I4_I8_MR), \
                    M_EXPAND_CONCAT(x, M_GEMM_I4_I8_NR))

#define M_GEMM_I2_I8_KER \
    M_EXPAND_CONCAT(M_EXPAND_CONCAT(m_gemm_ukr_i2_i8_mr, M_GEMM_I2_I8_MR), \
                    M_EXPAND_CONCAT(x, M_GEMM_I2_I8_NR))

_Static_assert(
    (M_GEMM_I8_I8_MR == 4) && (M_GEMM_I8_I8_NR == 2),
    "M_GEMM_I8_I8_MRxNR: no kernel implemented for this MRxNR"
);

_Static_assert(
    (M_GEMM_I4_I8_MR == 4) && (M_GEMM_I4_I8_NR == 2),
    "M_GEMM_I4_I8_MRxNR: no kernel implemented for this MRxNR"
);

_Static_assert(
    (M_GEMM_I2_I8_MR == 4) && (M_GEMM_I2_I8_NR == 2),
    "M_GEMM_I2_I8_MRxNR: no kernel implemented for this MRxNR"
);

// -----------------------------------------------------------------------------
// level-3: matrix-matrix product driver
// -----------------------------------------------------------------------------

void m_gemm_i8_i8(
    const size_t m, const size_t n, const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t);

void m_gemm_i4_i8(
    const size_t m, const size_t n, const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t);

void m_gemm_i2_i8(
    const size_t m, const size_t n, const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t);

#ifdef __cplusplus
}
#endif

#endif // COMMON_MATH_H

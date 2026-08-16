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

// -----------------------------------------------------------------------------
// level-1f: fused dot products
// -----------------------------------------------------------------------------
// m_dotf_<ta>_<tx>_mr<MR> computes MR dot products sharing one x vector:
//
//   y[i] = sum_{p<k} a[i*lda + p] * x[p],  i in [0, MR)
//
// the type suffixes are (matrix, vector),
// which is the reverse of dotv's (wide, narrow):
// the same operand pair is m_dotv_i8_i4 but m_dotf_i4_i8
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
// level-2: matrix-vector product driver
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
// m_gemm_ukr_<ta>_<tx>_mr<MR>x<NR> computes MRxNR dot products
//
//   c[i*ldc + j] = sum_{p<k} a[i*lda + p] * b[j*ldb + p],
//       i in [0, MR), j in [0, NR)
//
// the type suffixes are (matrix, vector),
// which is the reverse of dotv's (wide, narrow):
// the same operand pair is m_dotv_i8_i4 but m_dotf_i4_i8
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

// -----------------------------------------------------------------------------
// data formatting: single block transpose
// -----------------------------------------------------------------------------
// m_txp_<B>x<B>_<t> transposes one B x B block of t type:
//
//   c[j*ldc + i] = a[i*lda + j],  i, j in [0, B)
//
// -----------------------------------------------------------------------------

void m_txp_2x2_i16(
    const int16_t* a, const size_t lda, int16_t* c, const size_t ldc);
void m_txp_4x4_i8(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc);
void m_txp_8x8_i4(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc);
void m_txp_16x16_i2(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc);

// -----------------------------------------------------------------------------
// data formatting: block transpose driver
// -----------------------------------------------------------------------------

#define M_TXP_I16_BLK 2
#define M_TXP_I8_BLK 4
#define M_TXP_I4_BLK 8
#define M_TXP_I2_BLK 16

void m_txp_i16(
    const size_t m, const size_t n,
    const int16_t* a, const size_t lda, int16_t* c, const size_t ldc);
void m_txp_i8(
    const size_t m, const size_t n,
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc);
void m_txp_i4(
    const size_t m, const size_t n,
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc);
void m_txp_i2(
    const size_t m, const size_t n,
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc);

#ifdef __cplusplus
}
#endif

#endif // COMMON_MATH_H

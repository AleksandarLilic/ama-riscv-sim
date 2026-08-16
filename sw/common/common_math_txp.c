#include "common_math.h"

// transpose drivers
//
// (m % BLK) == 0 and (n % BLK) are requirements // FIXME: add tails
//
// block column is exactly one word wide for every type,
// so all pointers advance by 4 bytes per step

INLINE
void m_txp_i16(
    const size_t m, const size_t n,
    const int16_t* a, const size_t lda, int16_t* c, const size_t ldc)
{
    // inner loop on C-contiguous:
    // dirty-line writebacks on C cost more than loads-miss on A
    const int16_t* aj = a;
    int16_t* cj = c;
    for (size_t j = 0; j < n; j += M_TXP_I16_BLK) {
        const int16_t* ai = aj;
        int16_t* ci = cj;
        for (size_t i = 0; i < m; i += M_TXP_I16_BLK) {
            m_txp_2x2_i16(ai, lda, ci, ldc);
            ai += (M_TXP_I16_BLK * lda);
            ci += M_TXP_I16_BLK;
        }
        aj += M_TXP_I16_BLK;
        cj += (M_TXP_I16_BLK * ldc);
    }

    // inner loop on A-contiguous:
    // if dcache fits entire dataset and loop is unrolled by GCC, it's faster,
    // though it is an uncommon case for the most part
    /*
    const int16_t* ai = a;
    int16_t* ci = c;

    for (size_t i = 0; i < m; i += M_TXP_I16_BLK) {
        const int16_t* aj = ai;
        int16_t* cj = ci;
        for (size_t j = 0; j < n; j += M_TXP_I16_BLK) {
            m_txp_2x2_i16(aj, lda, cj, ldc);
            aj += M_TXP_I16_BLK;
            cj += (M_TXP_I16_BLK * ldc);
        }
        ai += (M_TXP_I16_BLK * lda);
        ci += M_TXP_I16_BLK;
    }
    */
}

INLINE
void m_txp_i8(
    const size_t m, const size_t n,
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    const int8_t* aj = a;
    int8_t* cj = c;
    for (size_t j = 0; j < n; j += M_TXP_I8_BLK) {
        const int8_t* ai = aj;
        int8_t* ci = cj;
        for (size_t i = 0; i < m; i += M_TXP_I8_BLK) {
            m_txp_4x4_i8(ai, lda, ci, ldc);
            ai += (M_TXP_I8_BLK * lda);
            ci += M_TXP_I8_BLK;
        }
        aj += M_TXP_I8_BLK;
        cj += (M_TXP_I8_BLK * ldc);
    }

    /*
    const int8_t* ai = a;
    int8_t* ci = c;

    for (size_t i = 0; i < m; i += M_TXP_I8_BLK) {
        const int8_t* aj = ai;
        int8_t* cj = ci;
        for (size_t j = 0; j < n; j += M_TXP_I8_BLK) {
            m_txp_4x4_i8(aj, lda, cj, ldc);
            aj += M_TXP_I8_BLK;
            cj += (M_TXP_I8_BLK * ldc);
        }
        ai += (M_TXP_I8_BLK * lda);
        ci += M_TXP_I8_BLK;
    }
    */
}

INLINE
void m_txp_i4(
    const size_t m, const size_t n,
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    const size_t lda_b = (lda >> 1); // row strides in bytes
    const size_t ldc_b = (ldc >> 1);

    const int8_t* aj = a;
    int8_t* cj = c;
    for (size_t j = 0; j < n; j += M_TXP_I4_BLK) {
        const int8_t* ai = aj;
        int8_t* ci = cj;
        for (size_t i = 0; i < m; i += M_TXP_I4_BLK) {
            m_txp_8x8_i4(ai, lda, ci, ldc);
            ai += (M_TXP_I4_BLK * lda_b);
            ci += (M_TXP_I4_BLK >> 1);
        }
        aj += (M_TXP_I4_BLK >> 1);
        cj += (M_TXP_I4_BLK * ldc_b);
    }

    /*
    const int8_t* ai = a;
    int8_t* ci = c;
    for (size_t i = 0; i < m; i += M_TXP_I4_BLK) {
        const int8_t* aj = ai;
        int8_t* cj = ci;
        for (size_t j = 0; j < n; j += M_TXP_I4_BLK) {
            m_txp_8x8_i4(aj, lda, cj, ldc);
            aj += (M_TXP_I4_BLK >> 1);
            cj += (M_TXP_I4_BLK * ldc_b);
        }
        ai += (M_TXP_I4_BLK * lda_b);
        ci += (M_TXP_I4_BLK >> 1);
    }
    */
}

INLINE
void m_txp_i2(
    const size_t m, const size_t n,
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    const size_t lda_b = (lda >> 2); // row strides in bytes
    const size_t ldc_b = (ldc >> 2);

    const int8_t* aj = a;
    int8_t* cj = c;
    for (size_t j = 0; j < n; j += M_TXP_I2_BLK) {
        const int8_t* ai = aj;
        int8_t* ci = cj;
        for (size_t i = 0; i < m; i += M_TXP_I2_BLK) {
            m_txp_16x16_i2(ai, lda, ci, ldc);
            ai += (M_TXP_I2_BLK * lda_b);
            ci += (M_TXP_I2_BLK >> 2);
        }
        aj += (M_TXP_I2_BLK >> 2);
        cj += (M_TXP_I2_BLK * ldc_b);
    }

    /*
    const int8_t* ai = a;
    int8_t* ci = c;
    for (size_t i = 0; i < m; i += M_TXP_I2_BLK) {
        const int8_t* aj = ai;
        int8_t* cj = ci;
        for (size_t j = 0; j < n; j += M_TXP_I2_BLK) {
            m_txp_16x16_i2(aj, lda, cj, ldc);
            aj += (M_TXP_I2_BLK >> 2);
            cj += (M_TXP_I2_BLK * ldc_b);
        }
        ai += (M_TXP_I2_BLK * lda_b);
        ci += (M_TXP_I2_BLK >> 2);
    }
    */
}

#include "common_math.h"

#if !defined(__riscv_xsimd) && !defined(LOAD_OPT)

// one element at a time, sub-byte blocking for i4 and i2
// sign never enters a transpose - it is bit movement, so everything is unsigned

INLINE
void m_txp_2x2_i16(
    const int16_t* a, const size_t lda, int16_t* c, const size_t ldc)
{
    for (size_t i = 0; i < M_TXP_I16_BLK; i++) {
        for (size_t j = 0; j < M_TXP_I16_BLK; j++) {
            c[j*ldc + i] = a[i*lda + j];
        }
    }
}

INLINE
void m_txp_4x4_i8(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    for (size_t i = 0; i < M_TXP_I8_BLK; i++) {
        for (size_t j = 0; j < M_TXP_I8_BLK; j++) {
            c[j*ldc + i] = a[i*lda + j];
        }
    }
}

INLINE
void m_txp_8x8_i4(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    const size_t lda_b = (lda >> 1); // row strides in bytes
    const size_t ldc_b = (ldc >> 1);

    for (size_t i = 0; i < M_TXP_I4_BLK; i++) { // dst row = src col
        for (size_t jb = 0; jb < (M_TXP_I4_BLK >> 1); jb++) { // dst byte
            uint32_t d = 0;
            for (size_t l = 0; l < 2; l++) {
                const size_t j = ((jb << 1) + l); // dst col = src row
                const uint32_t s = (uint8_t)a[j*lda_b + (i >> 1)];
                d |= (((s >> ((i & 1) << 2)) & 0xF) << (l << 2));
            }
            c[i*ldc_b + jb] = (int8_t)d;
        }
    }
}

INLINE
void m_txp_16x16_i2(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    const size_t lda_b = (lda >> 2); // row strides in bytes
    const size_t ldc_b = (ldc >> 2);

    for (size_t i = 0; i < M_TXP_I2_BLK; i++) { // dst row = src col
        for (size_t jb = 0; jb < (M_TXP_I2_BLK >> 2); jb++) { // dst byte
            uint32_t d = 0;
            for (size_t l = 0; l < 4; l++) {
                const size_t j = ((jb << 2) + l); // dst col = src row
                const uint32_t s = (uint8_t)a[j*lda_b + (i >> 2)];
                d |= (((s >> ((i & 3) << 1)) & 0x3) << (l << 1));
            }
            c[i*ldc_b + jb] = (int8_t)d;
        }
    }
}

#endif

#include "common_math.h"

INLINE
void m_gemv_i8_i8(
    const size_t m, const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    const int8_t* ap = a;
    size_t i = 0;

    for (; i + M_DOTF_I8_I8_MR <= m; i += M_DOTF_I8_I8_MR) {
        M_DOTF_I8_I8_KER(k, ap, lda, x, y + i);
        ap += (M_DOTF_I8_I8_MR * lda);
    }

    // m % MR is the dimension the caller chose, so it is gemv's to own
    for (; i < m; i++, ap += lda) {
        y[i] = m_dotv_i8_i8(ap, x, k);
    }
}

INLINE
void m_gemv_i4_i8(
    const size_t m, const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    const size_t lda_b = (lda >> 1); // row stride of 'a', in bytes
    const int8_t* ap = a;
    size_t i = 0;

    // the kernel takes 'lda' in logical elements and shifts it itself
    for (; i + M_DOTF_I4_I8_MR <= m; i += M_DOTF_I4_I8_MR) {
        M_DOTF_I4_I8_KER(k, ap, lda, x, y + i);
        ap += (M_DOTF_I4_I8_MR * lda_b);
    }

    // note the operand swap - dotv names the wide type first
    for (; i < m; i++, ap += lda_b) {
        y[i] = m_dotv_i8_i4(x, ap, k);
    }
}

INLINE
void m_gemv_i2_i8(
    const size_t m, const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    const size_t lda_b = (lda >> 2); // row stride of 'a', in bytes
    const int8_t* ap = a;
    size_t i = 0;

    for (; i + M_DOTF_I2_I8_MR <= m; i += M_DOTF_I2_I8_MR) {
        M_DOTF_I2_I8_KER(k, ap, lda, x, y + i);
        ap += (M_DOTF_I2_I8_MR * lda_b);
    }

    // note the operand swap - dotv names the wide type first
    for (; i < m; i++, ap += lda_b) {
        y[i] = m_dotv_i8_i2(x, ap, k);
    }
}

#include "common_math.h"

INLINE
void m_gemm_i8_i8(
    const size_t m, const size_t n, const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t)
{
    const int8_t* ap = a;
    size_t mr = 0;

    for (; mr + M_GEMM_I8_I8_MR <= m; mr += M_GEMM_I8_I8_MR) {
        size_t nr = 0;
        for (; nr + M_GEMM_I8_I8_NR <= n; nr += M_GEMM_I8_I8_NR) {
            int32_t* c_ptr = c_t ? (c + nr*ldc + mr) : (c + mr*ldc + nr);
            M_GEMM_I8_I8_KER(k, ap, lda, (b + nr*ldb), ldb, c_ptr, ldc, c_t);
        }
        // n % NR: MR outputs at 1 n, through dotf
        for (; nr < n; nr++) {
            if (c_t) {
                M_DOTF_I8_I8_KER(k, ap, lda, (b + nr*ldb), (c + nr*ldc + mr));
            } else {
                for (size_t r = 0; r < M_GEMM_I8_I8_MR; r++) {
                    c[(mr + r)*ldc + nr] =
                        m_dotv_i8_i8((ap + r*lda), (b + nr*ldb), k);
                }
            }
        }
        ap += (M_GEMM_I8_I8_MR * lda);
    }

    // m % MR: no fused kernel fits in either mode - one dot at a time
    for (; mr < m; mr++, ap += lda) {
        for (size_t j = 0; j < n; j++) {
            const int32_t d = m_dotv_i8_i8(ap, b + j*ldb, k);
            if (c_t) c[j*ldc + mr] = d;
            else c[mr*ldc + j] = d;
        }
    }
}

INLINE
void m_gemm_i4_i8(
    const size_t m, const size_t n, const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t)
{
    const size_t lda_b = (lda >> 1); // row stride of 'a', in bytes
    const int8_t* ap = a;
    size_t mr = 0;

    for (; mr + M_GEMM_I4_I8_MR <= m; mr += M_GEMM_I4_I8_MR) {
        size_t nr = 0;
        for (; nr + M_GEMM_I4_I8_NR <= n; nr += M_GEMM_I4_I8_NR) {
            int32_t* c_ptr = c_t ? (c + nr*ldc + mr) : (c + mr*ldc + nr);
            M_GEMM_I4_I8_KER(k, ap, lda, (b + nr*ldb), ldb, c_ptr, ldc, c_t);
        }
        // n % NR: MR outputs at 1 n, through dotf
        for (; nr < n; nr++) {
            if (c_t) {
                M_DOTF_I4_I8_KER(k, ap, lda, (b + nr*ldb), (c + nr*ldc + mr));
            } else {
                for (size_t r = 0; r < M_GEMM_I4_I8_MR; r++) {
                    c[(mr + r)*ldc + nr] =
                        m_dotv_i8_i4((b + nr*ldb), (ap + r*lda_b), k);
                }
            }
        }
        ap += (M_GEMM_I4_I8_MR * lda_b);
    }

    // m % MR: no fused kernel fits in either mode - one dot at a time
    for (; mr < m; mr++, ap += lda_b) {
        for (size_t j = 0; j < n; j++) {
            const int32_t d = m_dotv_i8_i4(b + j*ldb, ap, k);
            if (c_t) c[j*ldc + mr] = d;
            else c[mr*ldc + j] = d;
        }
    }
}

INLINE
void m_gemm_i2_i8(
    const size_t m, const size_t n, const size_t k,
    const int8_t* a, const size_t lda,
    const int8_t* b, const size_t ldb,
    int32_t* c, const size_t ldc,
    const bool c_t)
{
    const size_t lda_b = (lda >> 2); // row stride of 'a', in bytes
    const int8_t* ap = a;
    size_t mr = 0;

    for (; mr + M_GEMM_I2_I8_MR <= m; mr += M_GEMM_I2_I8_MR) {
        size_t nr = 0;
        for (; nr + M_GEMM_I2_I8_NR <= n; nr += M_GEMM_I2_I8_NR) {
            int32_t* c_ptr = c_t ? (c + nr*ldc + mr) : (c + mr*ldc + nr);
            M_GEMM_I2_I8_KER(k, ap, lda, (b + nr*ldb), ldb, c_ptr, ldc, c_t);
        }
        // n % NR: MR outputs at 1 n, through dotf
        for (; nr < n; nr++) {
            if (c_t) {
                M_DOTF_I2_I8_KER(k, ap, lda, (b + nr*ldb), (c + nr*ldc + mr));
            } else {
                for (size_t r = 0; r < M_GEMM_I2_I8_MR; r++) {
                    c[(mr + r)*ldc + nr] =
                        m_dotv_i8_i2((b + nr*ldb), (ap + r*lda_b), k);
                }
            }
        }
        ap += (M_GEMM_I2_I8_MR * lda_b);
    }

    // m % MR: no fused kernel fits in either mode - one dot at a time
    for (; mr < m; mr++, ap += lda_b) {
        for (size_t j = 0; j < n; j++) {
            const int32_t d = m_dotv_i8_i2(b + j*ldb, ap, k);
            if (c_t) c[j*ldc + mr] = d;
            else c[mr*ldc + j] = d;
        }
    }
}

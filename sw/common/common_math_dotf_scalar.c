#include "common_math.h"

#if !defined(__riscv_xsimd) && !defined(LOAD_OPT)

INLINE
void m_dotf_i8_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    #define K_ATOMIC 1 // one mul
    #define K_UNROLL 4
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // x[kk + off] is loaded once and reused by all MR rows
    #define MAC_A_ROW(r, off) c[r] += a[(r)*lda + kk + (off)] * xv[off]

    #define MAC_ALL_ROWS(off) \
        MAC_A_ROW(0, off); \
        MAC_A_ROW(1, off); \
        MAC_A_ROW(2, off); \
        MAC_A_ROW(3, off)

    int32_t c[4] = {0};
    const size_t kms = (k / K_STEP) * K_STEP;

    for (size_t kk = 0; kk < kms; kk += K_STEP) {
        int32_t xv[4];
        xv[0] = x[kk + 0];
        xv[1] = x[kk + 1];
        xv[2] = x[kk + 2];
        xv[3] = x[kk + 3];

        MAC_ALL_ROWS(0);
        MAC_ALL_ROWS(1);
        MAC_ALL_ROWS(2);
        MAC_ALL_ROWS(3);
    }

    // k tail: reuse dotv, which has its own tail
    if (kms < k) {
        for (size_t i = 0; i < 4; i++) {
            c[i] += m_dotv_i8_i8((a + i*lda + kms), (x + kms), (k - kms));
        }
    }

    y[0] = c[0];
    y[1] = c[1];
    y[2] = c[2];
    y[3] = c[3];

    #undef MAC_ALL_ROWS
    #undef MAC_A_ROW
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

INLINE
void m_dotf_i4_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    // K_ATOMIC = 2 (one packed byte of 'a'), K_UNROLL = 1 -> K_STEP = 2
    #define MAC_A_ROW(r) \
        do { \
            int8_t al = ap[(r)*lda_b]; \
            c[r] += (int8_t)(al >> 4) * x1; /* high nibble -> element j+1 */ \
            al <<= 4; \
            c[r] += (int8_t)(al >> 4) * x0; /* low nibble  -> element j   */ \
        } while (0)

    int32_t c[4] = {0};
    const size_t lda_b = (lda >> 1); // row stride of 'a', in bytes
    const int8_t* ap = a;

    for (size_t j = 0; j < k; j += 2, ap += 1) {
        const int32_t x0 = x[j];
        const int32_t x1 = x[j + 1];

        MAC_A_ROW(0);
        MAC_A_ROW(1);
        MAC_A_ROW(2);
        MAC_A_ROW(3);
    }

    y[0] = c[0];
    y[1] = c[1];
    y[2] = c[2];
    y[3] = c[3];

    #undef MAC_A_ROW
}

INLINE
void m_dotf_i2_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    // K_ATOMIC = 4 (one packed byte of 'a'), K_UNROLL = 1 -> K_STEP = 4
    #define MAC_A_ROW(r) \
        do { \
            int8_t al = ap[(r)*lda_b]; \
            c[r] += (int8_t)(al >> 6) * x3; /* bits 7:6 -> element j+3 */ \
            al <<= 2; \
            c[r] += (int8_t)(al >> 6) * x2; \
            al <<= 2; \
            c[r] += (int8_t)(al >> 6) * x1; \
            al <<= 2; \
            c[r] += (int8_t)(al >> 6) * x0; /* bits 1:0 -> element j   */ \
        } while (0)

    int32_t c[4] = {0};
    const size_t lda_b = (lda >> 2); // row stride of 'a', in bytes
    const int8_t* ap = a;

    for (size_t j = 0; j < k; j += 4, ap += 1) {
        const int32_t x0 = x[j];
        const int32_t x1 = x[j + 1];
        const int32_t x2 = x[j + 2];
        const int32_t x3 = x[j + 3];

        MAC_A_ROW(0);
        MAC_A_ROW(1);
        MAC_A_ROW(2);
        MAC_A_ROW(3);
    }

    y[0] = c[0];
    y[1] = c[1];
    y[2] = c[2];
    y[3] = c[3];

    #undef MAC_A_ROW
}

#endif // !__riscv_xsimd && !LOAD_OPT

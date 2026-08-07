#include "common_math.h"

#if !defined(__riscv_xsimd) && !defined(LOAD_OPT)

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
    #define K_ATOMIC 1 // one mul
    #define K_UNROLL 2
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // one 'a' element feeds NR macs
    // and one 'b' element feeds MR
    // 'a' load scheduling is fine, _a is loaded in advance by gcc already
    #define MAC_A_ROW(r, off) \
        do { \
            const int32_t _a = a[(r)*lda + kk + (off)]; \
            co[r][0] += _a * bv[0][off]; \
            co[r][1] += _a * bv[1][off]; \
        } while (0)

    #define MAC_ALL_ROWS(off) \
        MAC_A_ROW(0, off); \
        MAC_A_ROW(1, off); \
        MAC_A_ROW(2, off); \
        MAC_A_ROW(3, off)

    int32_t co[4][2] = {0};
    const size_t kms = (k / K_STEP) * K_STEP;

    for (size_t kk = 0; kk < kms; kk += K_STEP) {
        int32_t bv[2][2];
        bv[0][0] = b[0*ldb + kk + 0];
        bv[0][1] = b[0*ldb + kk + 1];
        bv[1][0] = b[1*ldb + kk + 0];
        bv[1][1] = b[1*ldb + kk + 1];

        MAC_ALL_ROWS(0);
        MAC_ALL_ROWS(1);
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
    #undef MAC_ALL_ROWS
    #undef MAC_A_ROW
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
    #define K_ATOMIC 2 // one packed byte of 'a'
    #define K_UNROLL 1
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // widened 'a' (two int4 elements) reused across 2 rows of 'b'
    #define MAC_A_ROW(r) \
        do { \
            int8_t al = ap[(r)*lda_b]; \
            const int32_t _ah = (int8_t)(al >> 4); /* high nibble -> kk+1 */ \
            al <<= 4; \
            const int32_t _al = (int8_t)(al >> 4); /* low nibble  -> kk   */ \
            co[r][0] += _ah * bv[0][1]; \
            co[r][1] += _ah * bv[1][1]; \
            co[r][0] += _al * bv[0][0]; \
            co[r][1] += _al * bv[1][0]; \
        } while (0)

    int32_t co[4][2] = {0};
    const size_t lda_b = (lda >> 1); // row stride of 'a', in bytes
    const int8_t* ap = a;

    // no k tail: 'a' is packed 2 per byte so k is always a multiple of K_STEP
    for (size_t kk = 0; kk < k; kk += K_STEP, ap += 1) {
        int32_t bv[2][2];
        bv[0][0] = b[0*ldb + kk + 0];
        bv[0][1] = b[0*ldb + kk + 1];
        bv[1][0] = b[1*ldb + kk + 0];
        bv[1][1] = b[1*ldb + kk + 1];

        MAC_A_ROW(0);
        MAC_A_ROW(1);
        MAC_A_ROW(2);
        MAC_A_ROW(3);
    }
    // store back
    STORE_C

    #undef NR
    #undef MAC_A_ROW
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
    #define K_ATOMIC 4 // one packed byte of 'a'
    #define K_UNROLL 1
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // one crumb, fed to NR macs; 'al' comes from the MAC_A_ROW wrapper
    #define MAC_CRUMB(r, e) \
        do { \
            const int32_t _a = (int8_t)(al >> 6); \
            co[r][0] += _a * bv[0][e]; \
            co[r][1] += _a * bv[1][e]; \
        } while (0)

    // crumbs walk descending from byte top: bits 7:6 are element kk+3
    #define MAC_A_ROW(r) \
        do { \
            int8_t al = ap[(r)*lda_b]; \
            MAC_CRUMB(r, 3); al <<= 2; \
            MAC_CRUMB(r, 2); al <<= 2; \
            MAC_CRUMB(r, 1); al <<= 2; \
            MAC_CRUMB(r, 0); \
        } while (0)

    int32_t co[4][2] = {0};
    const size_t lda_b = (lda >> 2); // row stride of 'a', in bytes
    const int8_t* ap = a;

    // no k tail: 'a' is packed 4 per byte so k is always a multiple of K_STEP
    for (size_t kk = 0; kk < k; kk += K_STEP, ap += 1) {
        int32_t bv[2][4];
        bv[0][0] = b[0*ldb + kk + 0];
        bv[0][1] = b[0*ldb + kk + 1];
        bv[0][2] = b[0*ldb + kk + 2];
        bv[0][3] = b[0*ldb + kk + 3];
        bv[1][0] = b[1*ldb + kk + 0];
        bv[1][1] = b[1*ldb + kk + 1];
        bv[1][2] = b[1*ldb + kk + 2];
        bv[1][3] = b[1*ldb + kk + 3];

        MAC_A_ROW(0);
        MAC_A_ROW(1);
        MAC_A_ROW(2);
        MAC_A_ROW(3);
    }
    // store back
    STORE_C

    #undef NR
    #undef MAC_A_ROW
    #undef MAC_CRUMB
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

#endif // !__riscv_xsimd && !LOAD_OPT

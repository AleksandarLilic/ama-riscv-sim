#include "common_math.h"

#if !defined(__riscv_xsimd) && defined(LOAD_OPT)

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
    #define K_ATOMIC 4 // load slice: 4 int8 per word
    #define K_UNROLL 1
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // nothing is unpacked ahead to reduce rf pressure:
    // all MR+NR words are held packed and the
    #define MAC_A_ROW(r, as) \
        do { \
            const int32_t _a = ((as) >> 24); \
            (as) <<= 8; \
            co[r][0] += _a * _b0; \
            co[r][1] += _a * _b1; \
        } while (0)

    int32_t co[4][2] = {0};
    const size_t kms = (k / K_STEP) * K_STEP;

    for (size_t kk = 0; kk < kms; kk += K_STEP) {
        int32_t as0 = *(const int32_t*)(a + 0*lda + kk);
        int32_t as1 = *(const int32_t*)(a + 1*lda + kk);
        int32_t as2 = *(const int32_t*)(a + 2*lda + kk);
        int32_t as3 = *(const int32_t*)(a + 3*lda + kk);
        int32_t bs0 = *(const int32_t*)(b + 0*ldb + kk);
        int32_t bs1 = *(const int32_t*)(b + 1*ldb + kk);

        for (size_t i = 0; i < K_ATOMIC; i++) {
            // one 'b' byte per n row, extracted once and fed to all MR macs
            const int32_t _b0 = (bs0 >> 24);
            const int32_t _b1 = (bs1 >> 24);
            bs0 <<= 8;
            bs1 <<= 8;

            MAC_A_ROW(0, as0);
            MAC_A_ROW(1, as1);
            MAC_A_ROW(2, as2);
            MAC_A_ROW(3, as3);
        }
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
    #define K_ATOMIC 8 // load slice: 8 int4 per word
    #define K_UNROLL 1
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // top nibble first, so the k elements are walked descending
    // and the 'b' words are taken in the same order;
    // one 'a' word spans 8 elements but 'b' word spans only 4
    #define MAC_A_ROW(r) \
        do { \
            const int32_t _a = (as[r] >> 28); \
            as[r] <<= 4; \
            co[r][0] += _a * _b0; \
            co[r][1] += _a * _b1; \
        } while (0)

    int32_t co[4][2] = {0};
    const size_t kms = (k / K_STEP) * K_STEP;
    const size_t lda_b = (lda >> 1); // row stride of 'a', in bytes
    const int8_t* ap = a;

    for (size_t kk = 0; kk < kms; kk += K_STEP, ap += (K_STEP >> 1)) {
        int32_t as[4];
        as[0] = *(const int32_t*)(ap + 0*lda_b);
        as[1] = *(const int32_t*)(ap + 1*lda_b);
        as[2] = *(const int32_t*)(ap + 2*lda_b);
        as[3] = *(const int32_t*)(ap + 3*lda_b);

        // descending: b word [4,8) first, then [0,4), matching the nibble walk
        for (size_t off = K_ATOMIC; off > 0; off -= 4) {
            int32_t bs0 = *(const int32_t*)(b + 0*ldb + kk + off - 4);
            int32_t bs1 = *(const int32_t*)(b + 1*ldb + kk + off - 4);
            for (size_t i = 0; i < 4; i++) {
                const int32_t _b0 = (bs0 >> 24);
                const int32_t _b1 = (bs1 >> 24);
                bs0 <<= 8;
                bs1 <<= 8;

                MAC_A_ROW(0);
                MAC_A_ROW(1);
                MAC_A_ROW(2);
                MAC_A_ROW(3);
            }
        }
    }

    // k tail: reuse dotv, which has its own tail
    // note the operand swap - dotv names the wide type first
    if (kms < k) {
        for (size_t i = 0; i < NR; i++) {
            const int8_t* b_ptr = (b + i*ldb + kms);
            const size_t kms_b = (kms >> 1);
            co[0][i] += m_dotv_i8_i4(b_ptr, (a + 0*lda_b + kms_b), (k - kms));
            co[1][i] += m_dotv_i8_i4(b_ptr, (a + 1*lda_b + kms_b), (k - kms));
            co[2][i] += m_dotv_i8_i4(b_ptr, (a + 2*lda_b + kms_b), (k - kms));
            co[3][i] += m_dotv_i8_i4(b_ptr, (a + 3*lda_b + kms_b), (k - kms));
        }
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
    #define K_ATOMIC 16 // load slice: 16 int2 per word
    #define K_UNROLL 1
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // top crumb first, so the k elements are walked descending
    // and the 'b' words are taken in the same order;
    // one 'a' word spans 16 elements but 'b' word spans only 4
    // quite the rf pressure due to 'a' side
    #define MAC_A_ROW(r) \
        do { \
            const int32_t _a = (as[r] >> 30); \
            as[r] <<= 2; \
            co[r][0] += _a * _b0; \
            co[r][1] += _a * _b1; \
        } while (0)

    int32_t co[4][2] = {0};
    const size_t kms = (k / K_STEP) * K_STEP;
    const size_t lda_b = (lda >> 2); // row stride of 'a', in bytes
    const int8_t* ap = a;

    for (size_t kk = 0; kk < kms; kk += K_STEP, ap += (K_STEP >> 2)) {
        int32_t as[4];
        as[0] = *(const int32_t*)(ap + 0*lda_b);
        as[1] = *(const int32_t*)(ap + 1*lda_b);
        as[2] = *(const int32_t*)(ap + 2*lda_b);
        as[3] = *(const int32_t*)(ap + 3*lda_b);

        // descending: b words [12,16), [8,12), [4,8), [0,4)
        for (size_t off = K_ATOMIC; off > 0; off -= 4) {
            int32_t bs0 = *(const int32_t*)(b + 0*ldb + kk + off - 4);
            int32_t bs1 = *(const int32_t*)(b + 1*ldb + kk + off - 4);
            for (size_t i = 0; i < 4; i++) {
                const int32_t _b0 = (bs0 >> 24);
                const int32_t _b1 = (bs1 >> 24);
                bs0 <<= 8;
                bs1 <<= 8;

                MAC_A_ROW(0);
                MAC_A_ROW(1);
                MAC_A_ROW(2);
                MAC_A_ROW(3);
            }
        }
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
    #undef MAC_A_ROW
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

#endif // !__riscv_xsimd && LOAD_OPT

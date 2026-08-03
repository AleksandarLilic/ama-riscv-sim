#include "common_math.h"

#if !defined(__riscv_xsimd) && defined(LOAD_OPT)

INLINE
void m_dotf_i8_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    #define K_ATOMIC 4 // load slice: 4 int8 per word
    #define K_UNROLL 1
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // one word of an 'a' row, MAC'd against the pre-extracted x bytes
    #define MAC_A_ROW(r) \
        do { \
            int32_t as = *(const int32_t*)(a + (r)*lda + kk); \
            for (size_t i = 0; i < K_ATOMIC; i++) { \
                c[r] += (as >> 24) * xv[i]; \
                as <<= 8; \
            } \
        } while (0)

    int32_t c[4] = {0};
    const size_t kms = (k / K_STEP) * K_STEP;

    for (size_t kk = 0; kk < kms; kk += K_STEP) {
        // x is loaded *and* unpacked once, then reused by all MR rows
        int32_t xs = *(const int32_t*)(x + kk);
        int32_t xv[K_ATOMIC];
        for (size_t i = 0; i < K_ATOMIC; i++) {
            xv[i] = (xs >> 24);
            xs <<= 8;
        }

        MAC_A_ROW(0);
        MAC_A_ROW(1);
        MAC_A_ROW(2);
        MAC_A_ROW(3);
    }

    if (kms < k) {
        for (size_t i = 0; i < 4; i++) {
            c[i] += m_dotv_i8_i8((a + i*lda + kms), (x + kms), (k - kms));
        }
    }

    y[0] = c[0];
    y[1] = c[1];
    y[2] = c[2];
    y[3] = c[3];

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
    #define K_ATOMIC 8 // load slice: 8 int4 per word
    #define K_UNROLL 1
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // 4 nibbles off the top of each row's word, against the 4 x lanes in hand
    #define MAC_A_ROW(r) \
        do { \
            for (size_t i = 0; i < 4; i++) { \
                c[r] += (as[r] >> 28) * xv[i]; \
                as[r] <<= 4; \
            } \
        } while (0)

    int32_t c[4] = {0};
    const size_t kms = (k / K_STEP) * K_STEP;
    const size_t lda_b = (lda >> 1); // row stride of 'a', in bytes
    const int8_t* ap = a;

    for (size_t kk = 0; kk < kms; kk += K_STEP, ap += (K_STEP >> 1)) {
        int32_t as[4];
        as[0] = *(const int32_t*)(ap + 0*lda_b);
        as[1] = *(const int32_t*)(ap + 1*lda_b);
        as[2] = *(const int32_t*)(ap + 2*lda_b);
        as[3] = *(const int32_t*)(ap + 3*lda_b);

        // descending: x word [4,8) first, then [0,4), matching the nibble walk
        for (size_t off = K_ATOMIC; off > 0; off -= 4) {
            int32_t xs = *(const int32_t*)(x + kk + off - 4);
            int32_t xv[4];
            for (size_t i = 0; i < 4; i++) {
                xv[i] = (xs >> 24);
                xs <<= 8;
            }

            MAC_A_ROW(0);
            MAC_A_ROW(1);
            MAC_A_ROW(2);
            MAC_A_ROW(3);
        }
    }

    // note the operand swap - dotv names the wide type first
    if (kms < k) {
        for (size_t i = 0; i < 4; i++) {
            c[i] += m_dotv_i8_i4(
                (x + kms), (a + i*(lda >> 1) + (kms >> 1)), (k - kms)
            );
        }
    }

    y[0] = c[0];
    y[1] = c[1];
    y[2] = c[2];
    y[3] = c[3];

    #undef MAC_A_ROW
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

INLINE
void m_dotf_i2_i8_mr4(
    const size_t k, const int8_t* a, const size_t lda,
    const int8_t* x, int32_t* y)
{
    #define K_ATOMIC 16 // load slice: 16 int2 per word
    #define K_UNROLL 1
    #define K_STEP   (K_ATOMIC * K_UNROLL)

    // one x word, unpacked once into named lanes and then reused by all MR rows
    #define UNPACK_X_GROUP(xs) \
        do { \
            int32_t _xs = (xs); \
            xv[0] = (_xs >> 24); _xs <<= 8; \
            xv[1] = (_xs >> 24); _xs <<= 8; \
            xv[2] = (_xs >> 24); _xs <<= 8; \
            xv[3] = (_xs >> 24); \
        } while (0)

    // 4 crumbs off the top of each row's word, against the 4 x lanes in hand
    #define MAC_A_ROW(r) \
        do { \
            c[r] += (as[r] >> 30) * xv[0]; as[r] <<= 2; \
            c[r] += (as[r] >> 30) * xv[1]; as[r] <<= 2; \
            c[r] += (as[r] >> 30) * xv[2]; as[r] <<= 2; \
            c[r] += (as[r] >> 30) * xv[3]; as[r] <<= 2; \
        } while (0)

    #define MAC_ALL_ROWS(xs) \
        UNPACK_X_GROUP(xs); \
        MAC_A_ROW(0); \
        MAC_A_ROW(1); \
        MAC_A_ROW(2); \
        MAC_A_ROW(3)

    int32_t c[4] = {0};
    const size_t kms = (k / K_STEP) * K_STEP;
    const size_t lda_b = (lda >> 2); // row stride of 'a', in bytes
    const int8_t* ap = a;

    for (size_t kk = 0; kk < kms; kk += K_STEP, ap += (K_STEP >> 2)) {
        // one word per row covers the whole step, so all MR are loaded
        int32_t xv[4], as[4];
        as[0] = *(const int32_t*)(ap + 0*lda_b);
        as[1] = *(const int32_t*)(ap + 1*lda_b);
        as[2] = *(const int32_t*)(ap + 2*lda_b);
        as[3] = *(const int32_t*)(ap + 3*lda_b);
        // x words too, so every load of the step is issued before the first
        // unpack reads one - unpacking straight off the load costs the 1 clk
        // load-to-use penalty once per group
        const int32_t xs3 = *(const int32_t*)(x + kk + 12);
        const int32_t xs2 = *(const int32_t*)(x + kk + 8);
        const int32_t xs1 = *(const int32_t*)(x + kk + 4);
        const int32_t xs0 = *(const int32_t*)(x + kk + 0);

        // descending, matching the crumb walk: x words [12,16) .. [0,4)
        MAC_ALL_ROWS(xs3);
        MAC_ALL_ROWS(xs2);
        MAC_ALL_ROWS(xs1);
        MAC_ALL_ROWS(xs0);
    }

    // note the operand swap - dotv names the wide type first
    if (kms < k) {
        for (size_t i = 0; i < 4; i++) {
            c[i] += m_dotv_i8_i2(
                (x + kms), (a + i*(lda >> 2) + (kms >> 2)), (k - kms)
            );
        }
    }

    y[0] = c[0];
    y[1] = c[1];
    y[2] = c[2];
    y[3] = c[3];

    #undef MAC_ALL_ROWS
    #undef MAC_A_ROW
    #undef UNPACK_X_GROUP
    #undef K_STEP
    #undef K_UNROLL
    #undef K_ATOMIC
}

#endif // !__riscv_xsimd && LOAD_OPT

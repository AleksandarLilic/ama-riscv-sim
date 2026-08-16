#include "common_math.h"

#if !defined(__riscv_xsimd) && defined(LOAD_OPT)

// SWAR diagonal-swap (Hacker's Delight 7-3, "Transposing a Bit Matrix")
// sign never enters a transpose - it is bit movement, so everything is unsigned

#define M_S2  0x33333333u
#define M_S4  0x0F0F0F0Fu
#define M_S8  0x00FF00FFu
#define M_S16 0x0000FFFFu

/*
def dswap_s(a0, a1, s, m):
    # secondary diagonal swap
    t = (a1 ^ (a0 >> s)) & m
    a1 = a1 ^ t
    a0 = a0 ^ (t << s)
    return a0, a1
*/

// secondary diagonal swap - same as simd's txp
#define DSWAP(i, j, s, m) \
    do { \
        const uint32_t _t = ((x[j] ^ (x[i] >> (s))) & (m)); \
        x[j] ^= _t; \
        x[i] ^= (_t << (s)); \
    } while (0)

#define LD_W(p) (*(const uint32_t*)(p))
#define ST_W(p, v) (*(uint32_t*)(p) = (v))

INLINE
void m_txp_2x2_i16(
    const int16_t* a, const size_t lda, int16_t* c, const size_t ldc)
{
    uint32_t x[2];
    x[0] = LD_W(a + 0*lda);
    x[1] = LD_W(a + 1*lda);

    DSWAP(0, 1, 16, M_S16);

    ST_W(c + 0*ldc, x[0]);
    ST_W(c + 1*ldc, x[1]);
}

INLINE
void m_txp_4x4_i8(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    uint32_t x[4];
    x[0] = LD_W(a + 0*lda);
    x[1] = LD_W(a + 1*lda);
    x[2] = LD_W(a + 2*lda);
    x[3] = LD_W(a + 3*lda);

    DSWAP(0, 1, 8, M_S8); DSWAP(2, 3, 8, M_S8); // bytes
    DSWAP(0, 2, 16, M_S16); DSWAP(1, 3, 16, M_S16); // halves

    ST_W(c + 0*ldc, x[0]);
    ST_W(c + 1*ldc, x[1]);
    ST_W(c + 2*ldc, x[2]);
    ST_W(c + 3*ldc, x[3]);
}

INLINE
void m_txp_8x8_i4(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    const size_t lda_b = (lda >> 1); // row strides in bytes
    const size_t ldc_b = (ldc >> 1);

    uint32_t x[8];
    x[0] = LD_W(a + 0*lda_b);
    x[1] = LD_W(a + 1*lda_b);
    x[2] = LD_W(a + 2*lda_b);
    x[3] = LD_W(a + 3*lda_b);
    x[4] = LD_W(a + 4*lda_b);
    x[5] = LD_W(a + 5*lda_b);
    x[6] = LD_W(a + 6*lda_b);
    x[7] = LD_W(a + 7*lda_b);

    // nibbles
    DSWAP(0, 1, 4, M_S4); DSWAP(2, 3, 4, M_S4);
    DSWAP(4, 5, 4, M_S4); DSWAP(6, 7, 4, M_S4);
    // bytes
    DSWAP(0, 2, 8, M_S8); DSWAP(1, 3, 8, M_S8);
    DSWAP(4, 6, 8, M_S8); DSWAP(5, 7, 8, M_S8);

    // halves, scheduling irrelevant, compiler will store as soon as possible
    DSWAP(0, 4, 16, M_S16);
    DSWAP(1, 5, 16, M_S16);
    DSWAP(2, 6, 16, M_S16);
    DSWAP(3, 7, 16, M_S16);

    ST_W(c + 0*ldc_b, x[0]);
    ST_W(c + 4*ldc_b, x[4]);
    ST_W(c + 1*ldc_b, x[1]);
    ST_W(c + 5*ldc_b, x[5]);
    ST_W(c + 2*ldc_b, x[2]);
    ST_W(c + 6*ldc_b, x[6]);
    ST_W(c + 3*ldc_b, x[3]);
    ST_W(c + 7*ldc_b, x[7]);
}

// 16 live words plus the temporaries and mask(s)
INLINE
void m_txp_16x16_i2(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    const size_t lda_b = (lda >> 2); // row strides in bytes
    const size_t ldc_b = (ldc >> 2);

    uint32_t x[16];
    x[0]  = LD_W(a +  0*lda_b);
    x[1]  = LD_W(a +  1*lda_b);
    x[2]  = LD_W(a +  2*lda_b);
    x[3]  = LD_W(a +  3*lda_b);
    x[4]  = LD_W(a +  4*lda_b);
    x[5]  = LD_W(a +  5*lda_b);
    x[6]  = LD_W(a +  6*lda_b);
    x[7]  = LD_W(a +  7*lda_b);
    x[8]  = LD_W(a +  8*lda_b);
    x[9]  = LD_W(a +  9*lda_b);
    x[10] = LD_W(a + 10*lda_b);
    x[11] = LD_W(a + 11*lda_b);
    x[12] = LD_W(a + 12*lda_b);
    x[13] = LD_W(a + 13*lda_b);
    x[14] = LD_W(a + 14*lda_b);
    x[15] = LD_W(a + 15*lda_b);

    // crumbs
    DSWAP(0, 1, 2, M_S2);     DSWAP(2, 3, 2, M_S2);
    DSWAP(4, 5, 2, M_S2);     DSWAP(6, 7, 2, M_S2);
    DSWAP(8, 9, 2, M_S2);     DSWAP(10, 11, 2, M_S2);
    DSWAP(12, 13, 2, M_S2);   DSWAP(14, 15, 2, M_S2);

    // nibbles
    DSWAP(0, 2, 4, M_S4);     DSWAP(1, 3, 4, M_S4);
    DSWAP(4, 6, 4, M_S4);     DSWAP(5, 7, 4, M_S4);
    DSWAP(8, 10, 4, M_S4);    DSWAP(9, 11, 4, M_S4);
    DSWAP(12, 14, 4, M_S4);   DSWAP(13, 15, 4, M_S4);

    // bytes
    DSWAP(0, 4, 8, M_S8);     DSWAP(1, 5, 8, M_S8);
    DSWAP(2, 6, 8, M_S8);     DSWAP(3, 7, 8, M_S8);
    DSWAP(8, 12, 8, M_S8);    DSWAP(9, 13, 8, M_S8);
    DSWAP(10, 14, 8, M_S8);   DSWAP(11, 15, 8, M_S8);

    // halves, scheduling irrelevant, compiler will store as soon as possible
    DSWAP(0, 8, 16, M_S16);   DSWAP(1, 9, 16, M_S16);
    DSWAP(2, 10, 16, M_S16);  DSWAP(3, 11, 16, M_S16);
    DSWAP(4, 12, 16, M_S16);  DSWAP(5, 13, 16, M_S16);
    DSWAP(6, 14, 16, M_S16);  DSWAP(7, 15, 16, M_S16);

    ST_W(c +  0*ldc_b, x[0]);
    ST_W(c +  1*ldc_b, x[1]);
    ST_W(c +  2*ldc_b, x[2]);
    ST_W(c +  3*ldc_b, x[3]);
    ST_W(c +  4*ldc_b, x[4]);
    ST_W(c +  5*ldc_b, x[5]);
    ST_W(c +  6*ldc_b, x[6]);
    ST_W(c +  7*ldc_b, x[7]);
    ST_W(c +  8*ldc_b, x[8]);
    ST_W(c +  9*ldc_b, x[9]);
    ST_W(c + 10*ldc_b, x[10]);
    ST_W(c + 11*ldc_b, x[11]);
    ST_W(c + 12*ldc_b, x[12]);
    ST_W(c + 13*ldc_b, x[13]);
    ST_W(c + 14*ldc_b, x[14]);
    ST_W(c + 15*ldc_b, x[15]);

}

#undef ST_W
#undef LD_W
#undef DSWAP
#undef M_S16
#undef M_S8
#undef M_S4
#undef M_S2

#endif

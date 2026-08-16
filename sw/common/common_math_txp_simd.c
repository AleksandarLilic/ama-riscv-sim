#include "common_math.h"
#include "common_math_simd_v_load_store.h"

#ifdef __riscv_xsimd

#define TXP(g, i, j) \
    do { \
        sliced64_t _p; \
        asm volatile( \
            "txp" #g " %0, %1, %2" \
            : "=r"(_p) \
            : "r"(x[i].v), "r"(x[j].v) \
        ); \
        x[i].v = _p.u32[0]; \
        x[j].v = _p.u32[1]; \
    } while (0)

INLINE
void m_txp_2x2_i16(
    const int16_t* a, const size_t lda, int16_t* c, const size_t ldc)
{
    int16x2_t x[2];
    x[0] = v_load_int16x2(a + 0*lda);
    x[1] = v_load_int16x2(a + 1*lda);

    TXP(16, 0, 1);

    v_store_int16x2(c + 0*ldc, x[0]);
    v_store_int16x2(c + 1*ldc, x[1]);
}

INLINE
void m_txp_4x4_i8(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    int8x4_t x[4];
    x[0] = v_load_int8x4(a + 0*lda);
    x[1] = v_load_int8x4(a + 1*lda);
    x[2] = v_load_int8x4(a + 2*lda);
    x[3] = v_load_int8x4(a + 3*lda);

    TXP(8, 0, 1);
    TXP(8, 2, 3);

    // halves, store early to reduce rf pressure
    TXP(16, 0, 2);
    v_store_int8x4(c + 0*ldc, x[0]);
    v_store_int8x4(c + 2*ldc, x[2]);
    TXP(16, 1, 3);
    v_store_int8x4(c + 1*ldc, x[1]);
    v_store_int8x4(c + 3*ldc, x[3]);
}

INLINE
void m_txp_8x8_i4(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    const size_t lda_b = (lda >> 1); // row strides in bytes
    const size_t ldc_b = (ldc >> 1);

    int4x8_t x[8];
    x[0] = v_load_int4x8(a + 0*lda_b);
    x[1] = v_load_int4x8(a + 1*lda_b);
    x[2] = v_load_int4x8(a + 2*lda_b);
    x[3] = v_load_int4x8(a + 3*lda_b);
    x[4] = v_load_int4x8(a + 4*lda_b);
    x[5] = v_load_int4x8(a + 5*lda_b);
    x[6] = v_load_int4x8(a + 6*lda_b);
    x[7] = v_load_int4x8(a + 7*lda_b);

    TXP(4, 0, 1); TXP(4, 2, 3); TXP(4, 4, 5); TXP(4, 6, 7); // nibbles
    TXP(8, 0, 2); TXP(8, 1, 3); TXP(8, 4, 6); TXP(8, 5, 7); // bytes

    // halves, store early to reduce rf pressure
    TXP(16, 0, 4);
    v_store_int4x8(c + 0*ldc_b, x[0]);
    v_store_int4x8(c + 4*ldc_b, x[4]);
    TXP(16, 1, 5);
    v_store_int4x8(c + 1*ldc_b, x[1]);
    v_store_int4x8(c + 5*ldc_b, x[5]);
    TXP(16, 2, 6);
    v_store_int4x8(c + 2*ldc_b, x[2]);
    v_store_int4x8(c + 6*ldc_b, x[6]);
    TXP(16, 3, 7);
    v_store_int4x8(c + 3*ldc_b, x[3]);
    v_store_int4x8(c + 7*ldc_b, x[7]);
}

// 16 live words plus the temporaries
INLINE
void m_txp_16x16_i2(
    const int8_t* a, const size_t lda, int8_t* c, const size_t ldc)
{
    const size_t lda_b = (lda >> 2); // row strides in bytes
    const size_t ldc_b = (ldc >> 2);

    int2x16_t x[16];
    x[0]  = v_load_int2x16(a +  0*lda_b);
    x[1]  = v_load_int2x16(a +  1*lda_b);
    x[2]  = v_load_int2x16(a +  2*lda_b);
    x[3]  = v_load_int2x16(a +  3*lda_b);
    x[4]  = v_load_int2x16(a +  4*lda_b);
    x[5]  = v_load_int2x16(a +  5*lda_b);
    x[6]  = v_load_int2x16(a +  6*lda_b);
    x[7]  = v_load_int2x16(a +  7*lda_b);
    x[8]  = v_load_int2x16(a +  8*lda_b);
    x[9]  = v_load_int2x16(a +  9*lda_b);
    x[10] = v_load_int2x16(a + 10*lda_b);
    x[11] = v_load_int2x16(a + 11*lda_b);
    x[12] = v_load_int2x16(a + 12*lda_b);
    x[13] = v_load_int2x16(a + 13*lda_b);
    x[14] = v_load_int2x16(a + 14*lda_b);
    x[15] = v_load_int2x16(a + 15*lda_b);

    // crumbs
    TXP(2, 0, 1);   TXP(2, 2, 3);   TXP(2, 4, 5);   TXP(2, 6, 7);
    TXP(2, 8, 9);   TXP(2, 10, 11); TXP(2, 12, 13); TXP(2, 14, 15);

    // nibbles
    TXP(4, 0, 2);   TXP(4, 1, 3);   TXP(4, 4, 6);   TXP(4, 5, 7);
    TXP(4, 8, 10);  TXP(4, 9, 11);  TXP(4, 12, 14); TXP(4, 13, 15);

    // bytes
    TXP(8, 0, 4);   TXP(8, 1, 5);   TXP(8, 2, 6);   TXP(8, 3, 7);
    TXP(8, 8, 12);  TXP(8, 9, 13);  TXP(8, 10, 14); TXP(8, 11, 15);

    // halves, store early to reduce rf pressure, some spill may still exist
    TXP(16, 0, 8);
    v_store_int2x16(c +  0*ldc_b, x[0]);
    v_store_int2x16(c +  8*ldc_b, x[8]);
    TXP(16, 1, 9);
    v_store_int2x16(c +  1*ldc_b, x[1]);
    v_store_int2x16(c +  9*ldc_b, x[9]);
    TXP(16, 2, 10);
    v_store_int2x16(c +  2*ldc_b, x[2]);
    v_store_int2x16(c + 10*ldc_b, x[10]);
    TXP(16, 3, 11);
    v_store_int2x16(c +  3*ldc_b, x[3]);
    v_store_int2x16(c + 11*ldc_b, x[11]);
    TXP(16, 4, 12);
    v_store_int2x16(c +  4*ldc_b, x[4]);
    v_store_int2x16(c + 12*ldc_b, x[12]);
    TXP(16, 5, 13);
    v_store_int2x16(c +  5*ldc_b, x[5]);
    v_store_int2x16(c + 13*ldc_b, x[13]);
    TXP(16, 6, 14);
    v_store_int2x16(c +  6*ldc_b, x[6]);
    v_store_int2x16(c + 14*ldc_b, x[14]);
    TXP(16, 7, 15);
    v_store_int2x16(c +  7*ldc_b, x[7]);
    v_store_int2x16(c + 15*ldc_b, x[15]);
}

#undef TXP

#endif

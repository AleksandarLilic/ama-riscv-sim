#include "common_math.h"
#include "common_math_dotv_scalar_core.h"

#if !defined(__riscv_xsimd) && !defined(LOAD_OPT)

#ifdef M_UNROLL_DOTV
INLINE_OPTION
int32_t m_dotv_i16_i16(const int16_t* a, const int16_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 4; // unroll degree
    static const size_t deg = (0 + udeg); // +0 for 1 el per core step
    const size_t tile = ((len >> deg) << deg);
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            const size_t ke = k + i;
            c += a[ke] * b[ke];
        }
    }

    const size_t rem = (len - tile);
    if (rem > 0) c += m_dotv_i16_i16_scalar_core(a + tile, b + tile, rem);
    return c;
}

INLINE_OPTION
int32_t m_dotv_i8_i8(const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 4; // unroll degree
    static const size_t deg = (0 + udeg); // +0 for 1 el per core step
    const size_t tile = ((len >> deg) << deg);
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            const size_t ke = k + i;
            c += a[ke] * b[ke];
        }
    }

    const size_t rem = (len - tile);
    if (rem > 0) c += m_dotv_i8_i8_scalar_core(a + tile, b + tile, rem);
    return c;
}

INLINE_OPTION
int32_t m_dotv_i4_i4(const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 3; // unroll degree
    static const size_t deg = (1 + udeg); // +1 for 2 el per core step
    const size_t tile = ((len >> deg) << deg); // 'len' and 'k' are in nibbles
    const int8_t* ap = a;
    const int8_t* bp = b;
    const size_t p_inc = (1 << deg);
    const size_t p_pinc = (p_inc >> 1); // packed increment, div 2 for i4

    for (size_t k = 0; k < tile; k += p_inc, ap += p_pinc, bp += p_pinc) {
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            int8_t al = ap[i];
            int8_t bl = bp[i];
            c += (int8_t)(al >> 4) * (int8_t)(bl >> 4);
            al <<= 4;
            bl <<= 4;
            c += (int8_t)(al >> 4) * (int8_t)(bl >> 4);
        }
    }

    const size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i4_i4_scalar_core(a + (tile >> 1), b + (tile >> 1), rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i2_i2(const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 2; // unroll degree
    static const size_t deg = (2 + udeg); // +2 for 4 el per core step
    const size_t tile = ((len >> deg) << deg); // 'len' and 'k' are in crumbs
    const int8_t* ap = a;
    const int8_t* bp = b;
    const size_t p_inc = (1 << deg);
    const size_t p_pinc = (p_inc >> 2); // packed increment, div 4 for i2

    for (size_t k = 0; k < tile; k += p_inc, ap += p_pinc, bp += p_pinc) {
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            int8_t al = ap[i];
            int8_t bl = bp[i];
            c += (int8_t)(al >> 6) * (int8_t)(bl >> 6);
            al <<= 2;
            bl <<= 2;
            c += (int8_t)(al >> 6) * (int8_t)(bl >> 6);
            al <<= 2;
            bl <<= 2;
            c += (int8_t)(al >> 6) * (int8_t)(bl >> 6);
            al <<= 2;
            bl <<= 2;
            c += (int8_t)(al >> 6) * (int8_t)(bl >> 6);
        }
    }

    const size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i2_i2_scalar_core(a + (tile >> 2), b + (tile >> 2), rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i8(const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 4; // unroll degree
    static const size_t deg = (0 + udeg); // +0 for 1 el per core step
    const size_t tile = ((len >> deg) << deg);
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            const size_t ke = k + i;
            c += a[ke] * (int16_t)b[ke];
        }
    }

    const size_t rem = (len - tile);
    if (rem > 0) c += m_dotv_i16_i8_scalar_core(a + tile, b + tile, rem);
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i4(const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 3; // unroll degree
    static const size_t deg = (1 + udeg); // +1 for 2 el per core step
    const size_t tile = ((len >> deg) << deg);
    const int8_t* bp = b;
    const size_t p_inc = (1 << deg);
    const size_t p_binc = (p_inc >> 1); // packed increment, div 2 for i4

    for (size_t k = 0; k < tile; k += p_inc, bp += p_binc) {
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            const size_t ke = k + i*2;
            int8_t bl = bp[i];
            c += a[ke + 1] * (int16_t)(bl >> 4);
            bl <<= 4;
            c += a[ke] * (int16_t)(bl >> 4);
        }
    }

    const size_t rem = (len - tile);
    if (rem > 0) c += m_dotv_i16_i4_scalar_core(a + tile, b + (tile >> 1), rem);
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i2(const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 2; // unroll degree
    static const size_t deg = (2 + udeg); // +2 for 4 el per core step
    const size_t tile = ((len >> deg) << deg);
    const int8_t* bp = b;
    const size_t p_inc = (1 << deg);
    const size_t p_binc = (p_inc >> 2); // packed increment, div 4 for i2

    for (size_t k = 0; k < tile; k += p_inc, bp += p_binc) {
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            const size_t ke = k + i*4;
            int8_t bl = bp[i];
            c += a[ke + 3] * (int16_t)(bl >> 6);
            bl <<= 2;
            c += a[ke + 2] * (int16_t)(bl >> 6);
            bl <<= 2;
            c += a[ke + 1] * (int16_t)(bl >> 6);
            bl <<= 2;
            c += a[ke] * (int16_t)(bl >> 6);
        }
    }

    const size_t rem = (len - tile);
    if (rem > 0) c += m_dotv_i16_i2_scalar_core(a + tile, b + (tile >> 2), rem);
    return c;
}

INLINE_OPTION
int32_t m_dotv_i8_i4(const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 3; // unroll degree
    static const size_t deg = (1 + udeg); // +1 for 2 el per core step
    const size_t tile = ((len >> deg) << deg);
    const int8_t* bp = b;
    const size_t p_inc = (1 << deg);
    const size_t p_binc = (p_inc >> 1); // packed increment, div 2 for i4

    for (size_t k = 0; k < tile; k += p_inc, bp += p_binc) {
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            const size_t ke = k + i*2;
            int8_t bl = bp[i];
            c += a[ke + 1] * (int8_t)(bl >> 4);
            bl <<= 4;
            c += a[ke] * (int8_t)(bl >> 4);
        }
    }

    const size_t rem = (len - tile);
    if (rem > 0) c += m_dotv_i8_i4_scalar_core(a + tile, b + (tile >> 1), rem);
    return c;
}

INLINE_OPTION
int32_t m_dotv_i8_i2(const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 2; // unroll degree
    static const size_t deg = (2 + udeg); // +2 for 4 el per core step
    const size_t tile = ((len >> deg) << deg);
    const int8_t* bp = b;
    const size_t p_inc = (1 << deg);
    const size_t p_binc = (p_inc >> 2); // packed increment, div 4 for i2

    for (size_t k = 0; k < tile; k += p_inc, bp += p_binc) {
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            const size_t ke = k + i*4;
            int8_t bl = bp[i];
            c += a[ke + 3] * (int8_t)(bl >> 6);
            bl <<= 2;
            c += a[ke + 2] * (int8_t)(bl >> 6);
            bl <<= 2;
            c += a[ke + 1] * (int8_t)(bl >> 6);
            bl <<= 2;
            c += a[ke] * (int8_t)(bl >> 6);
        }
    }

    const size_t rem = (len - tile);
    if (rem > 0) c += m_dotv_i8_i2_scalar_core(a + tile, b + (tile >> 2), rem);
    return c;
}

INLINE_OPTION
int32_t m_dotv_i4_i2(const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    static const size_t udeg = 2; // unroll degree
    static const size_t deg = (2 + udeg); // +2 for 4 el per core step
    const size_t tile = ((len >> deg) << deg);
    const int8_t* ap = a;
    const int8_t* bp = b;
    const size_t p_inc = (1 << deg);
    const size_t p_ainc = (p_inc >> 1); // packed a increment, div 2 for i4
    const size_t p_binc = (p_inc >> 2); // packed b increment, div 4 for i2

    for (size_t k = 0; k < tile; k += p_inc, ap += p_ainc, bp += p_binc) {
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            int8_t al = ap[i*2 + 1];
            int8_t bl = bp[i];
            c += (int8_t)(al >> 4) * (int8_t)(bl >> 6);
            al <<= 4;
            bl <<= 2;
            c += (int8_t)(al >> 4) * (int8_t)(bl >> 6);
            al = ap[i*2];
            bl <<= 2;
            c += (int8_t)(al >> 4) * (int8_t)(bl >> 6);
            al <<= 4;
            bl <<= 2;
            c += (int8_t)(al >> 4) * (int8_t)(bl >> 6);
        }
    }

    const size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i4_i2_scalar_core(a + (tile >> 1), b + (tile >> 2), rem);
    }
    return c;
}

#else // !M_UNROLL_DOTV

INLINE_OPTION
int32_t m_dotv_i16_i16(const int16_t* a, const int16_t* b, const size_t len)
{
    return m_dotv_i16_i16_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i8_i8(const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i8_i8_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i4_i4(const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i4_i4_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i2_i2(const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i2_i2_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i16_i8(const int16_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i16_i8_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i16_i4(const int16_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i16_i4_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i16_i2(const int16_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i16_i2_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i8_i4(const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i8_i4_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i8_i2(const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i8_i2_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i4_i2(const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i4_i2_scalar_core(a, b, len);
}

#endif // M_UNROLL_DOTV

#endif // !__riscv_xsimd && !LOAD_OPT

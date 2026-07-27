
#include "common_math.h"
#include "common_math_scalar_core.h"

#if !defined(__riscv_xsimd) && defined(LOAD_OPT)

INLINE_OPTION
void m_add_i16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len)
{
    size_t len_s2 = ((len >> 1) << 1);
    int32_t c_slice;
    for (size_t k = 0; k < len_s2; k += 2) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        int16_t a_half, b_half;
        int32_t c_halves[2];
        for (size_t i = 0; i < 2; i++) {
            a_half = (a_slice >> 16) & 0xFFFF;
            b_half = (b_slice >> 16) & 0xFFFF;
            c_halves[1 - i] = a_half + b_half;
            a_slice <<= 16;
            b_slice <<= 16;
        }
        //c_slice = *(int32_t*)c_halves; // not optimal - still uses store half
        c_slice = (c_halves[1] & 0xFFFF) << 16 | (c_halves[0] & 0xFFFF);
        *(int32_t*)(c + k) = c_slice;
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void m_add_i8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len)
{
    // doesn't yield better performance than generic implementation
    size_t len_s4 = ((len >> 2) << 2);
    int32_t c_slice;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        int8_t a_byte, b_byte;
        int32_t c_bytes[4];
        for (size_t i = 0; i < 4; i++) {
            a_byte = (a_slice >> 24) & 0xFF;
            b_byte = (b_slice >> 24) & 0xFF;
            c_bytes[3 - i] = a_byte + b_byte;
            a_slice <<= 8;
            b_slice <<= 8;
        }
        c_slice = (c_bytes[3] & 0xFF) << 24 | (c_bytes[2] & 0xFF) << 16 |
                  (c_bytes[1] & 0xFF) << 8 | (c_bytes[0] & 0xFF);
        *(int32_t*)(c + k) = c_slice;
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void m_sub_i16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len)
{
    size_t len_s2 = ((len >> 1) << 1);
    int32_t c_slice;
    for (size_t k = 0; k < len_s2; k += 2) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        int16_t a_half, b_half;
        int32_t c_halves[2];
        for (size_t i = 0; i < 2; i++) {
            a_half = (a_slice >> 16) & 0xFFFF;
            b_half = (b_slice >> 16) & 0xFFFF;
            c_halves[1 - i] = a_half - b_half;
            a_slice <<= 16;
            b_slice <<= 16;
        }
        //c_slice = *(int32_t*)c_halves; // not optimal - still uses store half
        c_slice = (c_halves[1] & 0xFFFF) << 16 | (c_halves[0] & 0xFFFF);
        *(int32_t*)(c + k) = c_slice;
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] - b[i];
    }
}

INLINE_OPTION
void m_sub_i8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len)
{
    // doesn't yield better performance than generic implementation
    size_t len_s4 = ((len >> 2) << 2);
    int32_t c_slice;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        int8_t a_byte, b_byte;
        int32_t c_bytes[4];
        for (size_t i = 0; i < 4; i++) {
            a_byte = (a_slice >> 24) & 0xFF;
            b_byte = (b_slice >> 24) & 0xFF;
            c_bytes[3 - i] = a_byte - b_byte;
            a_slice <<= 8;
            b_slice <<= 8;
        }
        c_slice = (c_bytes[3] & 0xFF) << 24 | (c_bytes[2] & 0xFF) << 16 |
                  (c_bytes[1] & 0xFF) << 8 | (c_bytes[0] & 0xFF);
        *(int32_t*)(c + k) = c_slice;
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] - b[i];
    }
}

// mul and mul unsigned

// widening mul perfrorms badly on load opt due to packing overhead
// 16-bit purely matches scalar, 8-bit executes ~50% more instructions
// 'define LOAD_OPT_USE_SCALAR' to fall back to the scalar cores for these four

INLINE_OPTION
void m_mul_i16(const int16_t* a, const int16_t* b, int32_t* c, const size_t len)
{
    #ifdef LOAD_OPT_USE_SCALAR
    m_mul_i16_scalar_core(a, b, c, len);
    #else
    size_t len_s2 = ((len >> 1) << 1);
    for (size_t k = 0; k < len_s2; k += 2) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        // int32 output: one word per element, nothing to repack
        c[k + 1] = (a_slice >> 16) * (b_slice >> 16);
        c[k]     = (int16_t)a_slice * (int16_t)b_slice;
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        m_mul_i16_scalar_core(a + len_s2, b + len_s2, c + len_s2, rem);
    }
    #endif
}

INLINE_OPTION
void m_mul_i8(const int8_t* a, const int8_t* b, int16_t* c, const size_t len)
{
    #ifdef LOAD_OPT_USE_SCALAR
    m_mul_i8_scalar_core(a, b, c, len);
    #else
    size_t len_s4 = ((len >> 2) << 2);
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        int32_t p[4];
        for (size_t i = 0; i < 4; i++) {
            p[3 - i] = (a_slice >> 24) * (b_slice >> 24);
            a_slice <<= 8;
            b_slice <<= 8;
        }
        // int16 output: two results per word, repack
        *(int32_t*)(c + k)     = (p[1] << 16) | (p[0] & 0xFFFF);
        *(int32_t*)(c + k + 2) = (p[3] << 16) | (p[2] & 0xFFFF);
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        m_mul_i8_scalar_core(a + len_s4, b + len_s4, c + len_s4, rem);
    }
    #endif
}

INLINE_OPTION
void m_mul_u16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len)
{
    #ifdef LOAD_OPT_USE_SCALAR
    m_mul_u16_scalar_core(a, b, c, len);
    #else
    size_t len_s2 = ((len >> 1) << 1);
    for (size_t k = 0; k < len_s2; k += 2) {
        uint32_t a_slice = *(const uint32_t*)(a + k);
        uint32_t b_slice = *(const uint32_t*)(b + k);
        c[k + 1] = (a_slice >> 16) * (b_slice >> 16);
        c[k]     = (uint16_t)a_slice * (uint16_t)b_slice;
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        m_mul_u16_scalar_core(a + len_s2, b + len_s2, c + len_s2, rem);
    }
    #endif
}

INLINE_OPTION
void m_mul_u8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len)
{
    #ifdef LOAD_OPT_USE_SCALAR
    m_mul_u8_scalar_core(a, b, c, len);
    #else
    size_t len_s4 = ((len >> 2) << 2);
    for (size_t k = 0; k < len_s4; k += 4) {
        uint32_t a_slice = *(const uint32_t*)(a + k);
        uint32_t b_slice = *(const uint32_t*)(b + k);
        uint32_t p[4];
        for (size_t i = 0; i < 4; i++) {
            p[3 - i] = (a_slice >> 24) * (b_slice >> 24);
            a_slice <<= 8;
            b_slice <<= 8;
        }
        *(uint32_t*)(c + k)     = (p[1] << 16) | (p[0] & 0xFFFF);
        *(uint32_t*)(c + k + 2) = (p[3] << 16) | (p[2] & 0xFFFF);
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        m_mul_u8_scalar_core(a + len_s4, b + len_s4, c + len_s4, rem);
    }
    #endif
}

INLINE_OPTION
int32_t m_dotv_i16_i16(const int16_t* a, const int16_t* b, const size_t len){
    int32_t c = 0;
    size_t len_s2 = ((len >> 1) << 1);
    for (size_t k = 0; k < len_s2; k += 2) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        // Loop through each half in the 32-bit slice
        int16_t a_half, b_half;
        for (size_t i = 0; i < 2; i++) {
            a_half = a_slice >> 16;
            b_half = b_slice >> 16;
            c += a_half * b_half;
            a_slice <<= 16;
            b_slice <<= 16;
        }
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        c += m_dotv_i16_i16_scalar_core(a + len_s2, b + len_s2, rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i8_i8(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s4 = ((len >> 2) << 2);
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        // Loop through each byte in the 32-bit slice
        int8_t a_byte, b_byte;
        for (size_t i = 0; i < 4; i++) {
            a_byte = a_slice >> 24;
            b_byte = b_slice >> 24;
            c += a_byte * b_byte;
            a_slice <<= 8;
            b_slice <<= 8;
        }
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        c += m_dotv_i8_i8_scalar_core(a + len_s4, b + len_s4, rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i4_i4(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_bytes = (len >> 1); // len passed in as number of nibbles
    size_t len_s4 = ((len_bytes) >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        // Loop through each nibble in the 32-bit slice
        int8_t a_nibble, b_nibble;
        for (size_t i = 0; i < 8; i++) {
            a_nibble = (a_slice) >> 28;
            b_nibble = (b_slice) >> 28;
            c += a_nibble * b_nibble;
            a_slice <<= 4;
            b_slice <<= 4;
        }
    }
    size_t rem = len_bytes - len_s4;
    if (rem > 0) {
        c += m_dotv_i4_i4_scalar_core(a + len_s4, b + len_s4, rem << 1);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i2_i2(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_bytes = (len >> 2); // len passed in as number of crumbs
    size_t len_s4 = ((len_bytes) >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        // Loop through each crumb in the 32-bit slice
        int8_t a_crumb, b_crumb;
        for (size_t i = 0; i < 16; i++) {
            a_crumb = a_slice >> 30;
            b_crumb = b_slice >> 30;
            c += a_crumb * b_crumb;
            a_slice <<= 2;
            b_slice <<= 2;
        }
    }
    size_t rem = (len_bytes - len_s4);
    if (rem > 0) {
        c += m_dotv_i2_i2_scalar_core(a + len_s4, b + len_s4, rem << 2);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i8(
    const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s4 = ((len >> 2) << 2);
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t b_slice = *(const int32_t*)(b + k);
        for (size_t j = 0; j < 4; j += 2) {
            int32_t a_slice = *(const int32_t*)(a + k + 2 - j); // MSB first
            int8_t b_byte;
            int16_t a_half;
            for (size_t i = 0; i < 2; i++) {
                b_byte = b_slice >> 24;
                a_half = a_slice >> 16;
                c += a_half * (int16_t)b_byte;
                b_slice <<= 8;
                a_slice <<= 16;
            }
        }
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        c += m_dotv_i16_i8_scalar_core(a + len_s4, b + len_s4, rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i4(
    const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s8 = ((len >> 3) << 3);
    for (size_t k = 0; k < len_s8; k += 8) {
        int32_t b_slice = *(const int32_t*)(b + (k >> 1));
        for (size_t j = 0; j < 8; j += 2) {
            int32_t a_slice = *(const int32_t*)(a + k + 6 - j);
            int8_t b_nibble;
            int16_t a_half;
            for (size_t i = 0; i < 2; i++) {
                b_nibble = b_slice >> 28;
                a_half = a_slice >> 16;
                c += a_half * (int16_t)b_nibble;
                b_slice <<= 4;
                a_slice <<= 16;
            }
        }
    }
    size_t rem = (len - len_s8);
    if (rem > 0) {
        c += m_dotv_i16_i4_scalar_core(
            a + len_s8, b + (len_s8 >> 1), rem
        );
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i2(
    const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s16 = ((len >> 4) << 4);
    for (size_t k = 0; k < len_s16; k += 16) {
        int32_t b_slice = *(const int32_t*)(b + (k >> 2));
        for (size_t j = 0; j < 16; j += 2) {
            int32_t a_slice = *(const int32_t*)(a + k + 14 - j);
            int8_t b_crumb;
            int16_t a_half;
            for (size_t i = 0; i < 2; i++) {
                b_crumb = b_slice >> 30;
                a_half = a_slice >> 16;
                c += a_half * (int16_t)b_crumb;
                b_slice <<= 2;
                a_slice <<= 16;
            }
        }
    }
    size_t rem = (len - len_s16);
    if (rem > 0) {
        c += m_dotv_i16_i2_scalar_core(
            a + len_s16, b + (len_s16 >> 2), rem
        );
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i8_i4(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s8 = ((len >> 3) << 3);
    for (size_t k = 0; k < len_s8; k += 8) {
        int32_t b_slice = *(const int32_t*)(b + (k >> 1));
        for (size_t j = 0; j < 8; j += 4) {
            int32_t a_slice = *(const int32_t*)(a + k + 4 - j);
            int8_t b_nibble;
            int8_t a_byte;
            for (size_t i = 0; i < 4; i++) {
                b_nibble = b_slice >> 28;
                a_byte = a_slice >> 24;
                c += a_byte * (int8_t)b_nibble;
                b_slice <<= 4;
                a_slice <<= 8;
            }
        }
    }
    size_t rem = (len - len_s8);
    if (rem > 0) {
        c += m_dotv_i8_i4_scalar_core(
            a + len_s8, b + (len_s8 >> 1), rem
        );
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i8_i2(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s16 = ((len >> 4) << 4);
    for (size_t k = 0; k < len_s16; k += 16) {
        int32_t b_slice = *(const int32_t*)(b + (k >> 2));
        for (size_t j = 0; j < 16; j += 4) {
            int32_t a_slice = *(const int32_t*)(a + k + 12 - j);
            int8_t b_crumb;
            int8_t a_byte;
            for (size_t i = 0; i < 4; i++) {
                b_crumb = b_slice >> 30;
                a_byte = a_slice >> 24;
                c += a_byte * (int8_t)b_crumb;
                b_slice <<= 2;
                a_slice <<= 8;
            }
        }
    }
    size_t rem = (len - len_s16);
    if (rem > 0) {
        c += m_dotv_i8_i2_scalar_core(
            a + len_s16, b + (len_s16 >> 2), rem
        );
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i4_i2(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    size_t len_s16 = ((len >> 4) << 4);
    for (size_t k = 0; k < len_s16; k += 16) {
        int32_t b_slice = *(const int32_t*)(b + (k >> 2));
        for (size_t j = 0; j < 8; j += 4) {
            int32_t a_slice = *(const int32_t*)(a + (k >> 1) + 4 - j);
            int8_t b_crumb;
            int8_t a_nibble;
            for (size_t i = 0; i < 8; i++) {
                b_crumb = b_slice >> 30;
                a_nibble = a_slice >> 28;
                c += a_nibble * (int8_t)b_crumb;
                b_slice <<= 2;
                a_slice <<= 4;
            }
        }
    }
    size_t rem = (len - len_s16);
    if (rem > 0) {
        c += m_dotv_i4_i2_scalar_core(
            a + (len_s16 >> 1), b + (len_s16 >> 2), rem
        );
    }
    return c;
}

#endif

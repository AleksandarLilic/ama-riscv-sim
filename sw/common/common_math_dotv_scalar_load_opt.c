
#include "common_math.h"
#include "common_math_dotv_scalar_core.h"

#if !defined(__riscv_xsimd) && defined(LOAD_OPT)

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

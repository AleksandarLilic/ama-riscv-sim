
#include "common_math.h"
#include "common_math_dotv_scalar_core.h"

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
void m_wmul_i16(const int16_t* a, const int16_t* b, int32_t* c, const size_t len)
{
    #ifdef LOAD_OPT_USE_SCALAR
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
    #else
    size_t len_s2 = ((len >> 1) << 1);
    for (size_t k = 0; k < len_s2; k += 2) {
        int32_t a_slice = *(const int32_t*)(a + k);
        int32_t b_slice = *(const int32_t*)(b + k);
        // int32 output: one word per element, nothing to repack
        c[k + 1] = (a_slice >> 16) * (b_slice >> 16);
        c[k + 0] = (int16_t)a_slice * (int16_t)b_slice;
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] * b[i];
    }
    #endif
}

INLINE_OPTION
void m_wmul_i8(const int8_t* a, const int8_t* b, int16_t* c, const size_t len)
{
    #ifdef LOAD_OPT_USE_SCALAR
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
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
        *(int32_t*)(c + k + 0) = (p[1] << 16) | (p[0] & 0xFFFF);
        *(int32_t*)(c + k + 2) = (p[3] << 16) | (p[2] & 0xFFFF);
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] * b[i];
    }
    #endif
}

INLINE_OPTION
void m_wmul_u16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len)
{
    #ifdef LOAD_OPT_USE_SCALAR
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
    #else
    size_t len_s2 = ((len >> 1) << 1);
    for (size_t k = 0; k < len_s2; k += 2) {
        uint32_t a_slice = *(const uint32_t*)(a + k);
        uint32_t b_slice = *(const uint32_t*)(b + k);
        c[k + 1] = (a_slice >> 16) * (b_slice >> 16);
        c[k + 0] = (uint16_t)a_slice * (uint16_t)b_slice;
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] * b[i];
    }
    #endif
}

INLINE_OPTION
void m_wmul_u8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len)
{
    #ifdef LOAD_OPT_USE_SCALAR
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
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
        *(uint32_t*)(c + k + 0) = (p[1] << 16) | (p[0] & 0xFFFF);
        *(uint32_t*)(c + k + 2) = (p[3] << 16) | (p[2] & 0xFFFF);
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] * b[i];
    }
    #endif
}

#endif

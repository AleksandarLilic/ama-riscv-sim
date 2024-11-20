#include "common_math.h"

#define TO_U4(x) (uint8_t)(x & 0xF)
#define TO_I4(x) (int8_t)((x & 0xF) | ((x & 0x8) ? 0xF0 : 0x00))

int32_t dot_product_int16(const int16_t* a, const int16_t* b, const size_t len) {
    int32_t c = 0;

    #if defined(LOAD_OPT) || defined(CUSTOM_ISA)
    // 2 halves at a time
    for (size_t k = 0; k < (len >> 1) << 1; k += 2) {
        int32_t a_slice = *(int32_t*)(a + k);
        int32_t b_slice = *(int32_t*)(b + k);

        #ifdef CUSTOM_ISA
        c += fma16(a_slice, b_slice);

        #else
        // Loop through each half in the 32-bit slice
        int16_t a_half, b_half;
        for (size_t i = 0; i < 2; i++) {
            a_half = a_slice >> 16;
            b_half = b_slice >> 16;
            c += a_half * b_half;
            a_slice <<= 16;
            b_slice <<= 16;
        }
        #endif
    }

    // leftover halves, if any, no benefit in using simd
    size_t rem = len % 2;
    if (rem > 0) {
        for (size_t i = 0; i < rem; i++)
            c += a[len - rem + i] * b[len - rem + i];
    }

    #else
    // generic implementation
    for (size_t k = 0; k < len; k++) {
        c += a[k] * b[k];
    }
    #endif
    return c;
}

int32_t dot_product_int8(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;

    #if defined(LOAD_OPT) || defined(CUSTOM_ISA)
    // 4 bytes at a time
    for (size_t k = 0; k < (len >> 2) << 2; k += 4) {
        int32_t a_slice = *(int32_t*)(a + k);
        int32_t b_slice = *(int32_t*)(b + k);

        #ifdef CUSTOM_ISA
        c += fma8(a_slice, b_slice);

        #else
        // Loop through each byte in the 32-bit slice
        int8_t a_byte, b_byte;
        for (size_t i = 0; i < 4; i++) {
            a_byte = a_slice >> 24;
            b_byte = b_slice >> 24;
            c += a_byte * b_byte;
            a_slice <<= 8;
            b_slice <<= 8;
        }
        #endif
    }

    // leftover bytes, if any, no benefit in using simd
    size_t rem = len % 4;
    if (rem > 0) {
        for (size_t i = 0; i < rem; i++)
            c += a[len - rem + i] * b[len - rem + i];
    }

    #else
    // generic implementation
    for (size_t k = 0; k < len; k++) {
        c += a[k] * b[k];
    }
    #endif
    return c;
}

int32_t dot_product_int4(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;

    #if defined(LOAD_OPT) || defined(CUSTOM_ISA)
    // 8 nibbles at a time (as 4 bytes)
    for (size_t k = 0; k < (len >> 2) << 2; k += 4) {
        int32_t a_slice = *(int32_t*)(a + k);
        int32_t b_slice = *(int32_t*)(b + k);

        #ifdef CUSTOM_ISA
        c += fma4(a_slice, b_slice);

        #else
        // Loop through each nibble in the 32-bit slice
        int8_t a_nibble, b_nibble;
        for (size_t i = 0; i < 8; i++) {
            a_nibble = (a_slice) >> 28;
            b_nibble = (b_slice) >> 28;
            c += a_nibble * b_nibble;
            a_slice <<= 4;
            b_slice <<= 4;
        }
        #endif
    }

    // leftover nibbles, if any, no benefit in using simd
    size_t rem = len % 8;
    if (rem > 0) {
        for (size_t i = 0; i < rem; i++)
            c += a[len - rem + i] * b[len - rem + i];
    }

    #else
    // generic implementation
    uint32_t temp;
    int8_t al, bl;
    for (size_t k = 0; k < len; k++) {
        al = a[k];
        bl = b[k];
        c += (al>>4) * (bl>>4);
        al <<= 4;
        bl <<= 4;
        c += (al>>4) * (bl>>4);
    }
    #endif
    return c;
}

#ifdef CUSTOM_ISA
int32_t fma16(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("fma16 %0, %1, %2"
                 : "=r"(c)
                 : "r"(a), "r"(b));
    return c;
}

int32_t fma8(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("fma8 %0, %1, %2"
                 : "=r"(c)
                 : "r"(a), "r"(b));
    return c;
}

int32_t fma4(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("fma4 %0, %1, %2"
                 : "=r"(c)
                 : "r"(a), "r"(b));
    return c;
}
#endif

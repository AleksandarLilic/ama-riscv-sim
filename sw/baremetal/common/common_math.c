#include "common_math.h"

#ifdef CUSTOM_ISA
INLINE_OPTION int32_t _simd_dot_product_int16(
    const int16_t* a, const int16_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s2 = (len >> 1) << 1;
    for (size_t k = 0; k < len_s2; k += 2) {
        int32_t a_slice = *(int32_t*)(a + k);
        int32_t b_slice = *(int32_t*)(b + k);
        c += fma16(a_slice, b_slice);
    }
    size_t rem = len - len_s2;
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c += a[i] * b[i];
    }
    return c;
}

#ifdef STREAMING // optimized for pipeline utilization, 4x unrolling

INLINE_OPTION int32_t _simd_dot_product_int8(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s16 = (len >> 4) << 4;
    for (size_t k = 0; k < len_s16; k += 16) {
        int32_t a_arr[4], b_arr[4], p_arr[4];
        int32_t temp;

        #pragma GCC unroll 4
        for (size_t i = 0; i < 4; i++) {
            a_arr[i] = *(int32_t*)(a + k + i * 4);
            b_arr[i] = *(int32_t*)(b + k + i * 4);
        }
        asm volatile (
            "fma8 %[p1], %[a0], %[b0];"
            "fma8 %[p2], %[a1], %[b1];"
            "fma8 %[p3], %[a2], %[b2];"
            "fma8 %[p4], %[a3], %[b3];"
            "add %[t], %[p1], %[p2];"
            "add %[c], %[p3], %[p4];"
            "add %[c], %[c], %[t];"
            : [c] "+r" (c), [t] "+r" (temp),
              [p1] "+r" (p_arr[0]),
              [p2] "+r" (p_arr[1]),
              [p3] "+r" (p_arr[2]),
              [p4] "+r" (p_arr[3])
            : [a0] "r" (a_arr[0]), [b0] "r" (b_arr[0]),
              [a1] "r" (a_arr[1]), [b1] "r" (b_arr[1]),
              [a2] "r" (a_arr[2]), [b2] "r" (b_arr[2]),
              [a3] "r" (a_arr[3]), [b3] "r" (b_arr[3])
            :
        );
    }
    // can't stream 16 bytes at a time anymore, do 4 bytes instead, if any
    size_t len_s4 = ((len - len_s16) >> 2) << 2;
    if (len_s4 > 0) {
        size_t start = len_s16;
        size_t end = len_s16 + len_s4;
        for (size_t i = start; i < end; i += 4) {
            int32_t a_slice = *(int32_t*)(a + i);
            int32_t b_slice = *(int32_t*)(b + i);
            c += fma8(a_slice, b_slice);
        }
    }
    size_t rem = len - (len_s16 + len_s4);
    if (rem > 0) {
        for (size_t i = (len_s16 + len_s4); i < len; i++) c += a[i] * b[i];
    }
    return c;
}

#else // one SIMD inst at a time

INLINE_OPTION int32_t _simd_dot_product_int8(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s4 = (len >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t a_slice = *(int32_t*)(a + k);
        int32_t b_slice = *(int32_t*)(b + k);
        c += fma8(a_slice, b_slice);
    }
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c += a[i] * b[i];
    }
    return c;
}
#endif // STREAMING

INLINE_OPTION int32_t _simd_dot_product_int4(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_bytes = len >> 1; // len passed in as number of nibbles
    size_t len_s4 = ((len_bytes) >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t a_slice = *(int32_t*)(a + k);
        int32_t b_slice = *(int32_t*)(b + k);
        c += fma4(a_slice, b_slice);
    }
    size_t rem = len_bytes - len_s4;
    if (rem > 0) {
        int8_t al, bl;
        for (size_t i = len_s4; i < len_bytes; i++) {
            al = a[i];
            bl = b[i];
            c += (al >> 4) * (bl >> 4);
            al <<= 4;
            bl <<= 4;
            c += (al >> 4) * (bl >> 4);
        }
    }
    return c;
}

INLINE_OPTION int32_t _simd_dot_product_int16_int8(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s4 = (len >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t p1, p2;
        int32_t b_slice = *(int32_t*)(b + k);
        int32_t a_slice_1 = *(int32_t*)(a + k);
        int32_t a_slice_2 = *(int32_t*)(a + k + 2);
        asm (
            "unpk8 "TOSTR(RD_1)", %[b];"
            // x30 (RD_1) = 2 lower halves, x31 (RDP_1) = 2 upper halves
            "fma16 %[p1], %[a1], "TOSTR(RD_1)";"
            "fma16 %[p2], %[a2], "TOSTR(RDP_1)";"
            "add %[c], %[c], %[p1];"
            "add %[c], %[c], %[p2];"
            : [c] "+r" (c), [p1] "=r" (p1), [p2] "=r" (p2)
            : [b] "r" (b_slice),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
            : TOSTR(RDP_1), TOSTR(RD_1)
        );
    }
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c += a[i] * (int16_t)b[i];
    }
    return c;
}

INLINE_OPTION int32_t _simd_dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s8 = (len >> 3) << 3;
    for (size_t k = 0; k < len_s8; k += 8) {
        int32_t p1, p2, p3, p4;
        int32_t b_slice = *(int32_t*)(b + (k >> 1));
        int32_t a_slice_1 = *(int32_t*)(a + k);
        int32_t a_slice_2 = *(int32_t*)(a + k + 2);
        int32_t a_slice_3 = *(int32_t*)(a + k + 4);
        int32_t a_slice_4 = *(int32_t*)(a + k + 6);
        asm (
            "unpk4 "TOSTR(RD_1)", %[b];" // nibbles to bytes
            "unpk8 "TOSTR(RD_2)", "TOSTR(RD_1)";" // low bytes to halfwords
            "fma16 %[p1], %[a1], "TOSTR(RD_2)";"
            "fma16 %[p2], %[a2], "TOSTR(RDP_2)";"
            "add %[c], %[c], %[p1];"
            "add %[c], %[c], %[p2];"
            "unpk8 "TOSTR(RD_2)", "TOSTR(RDP_1)";" // high bytes to halfwords
            "fma16 %[p1], %[a3], "TOSTR(RD_2)";"
            "fma16 %[p2], %[a4], "TOSTR(RDP_2)";"
            "add %[c], %[c], %[p1];"
            "add %[c], %[c], %[p2];"
            : [c] "+r" (c),
              [p1] "+r" (p1), [p2] "+r" (p2)
            : [b] "r" (b_slice),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2),
              [a3] "r" (a_slice_3), [a4] "r" (a_slice_4)
            : TOSTR(RDP_1), TOSTR(RD_1), TOSTR(RDP_2), TOSTR(RD_2)
        );
    }
    size_t rem = len - len_s8;
    if (rem > 0) {
        int8_t bl;
        for (size_t i = len_s8; i < len; i += 2) {
            bl = b[i>>1];
            c += a[i+1] * (int16_t)(bl >> 4);
            bl <<= 4;
            c += a[i] * (int16_t)(bl >> 4);
        }
    }
    return c;
}

INLINE_OPTION int32_t _simd_dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s8 = (len >> 3) << 3;
    for (size_t k = 0; k < len_s8; k += 8) {
        int32_t p1, p2;
        int32_t b_slice = *(int32_t*)(b + (k >> 1));
        int32_t a_slice_1 = *(int32_t*)(a + k);
        int32_t a_slice_2 = *(int32_t*)(a + k + 4);
        asm (
            "unpk4 "TOSTR(RD_1)", %[b];"
            "fma8 %[p1], %[a1], "TOSTR(RD_1)";"
            "fma8 %[p2], %[a2], "TOSTR(RDP_1)";"
            "add %[c], %[c], %[p1];"
            "add %[c], %[c], %[p2];"
            : [c] "+r" (c), [p1] "=r" (p1), [p2] "=r" (p2)
            : [b] "r" (b_slice),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
            : TOSTR(RDP_1), TOSTR(RD_1)
        );
    }
    size_t rem = len - len_s8;
    if (rem > 0) {
        int8_t bl;
        for (size_t i = len_s8; i < len; i += 2) {
            bl = b[i>>1];
            c += a[i+1] * (int8_t)(bl >> 4);
            bl <<= 4;
            c += a[i] * (int8_t)(bl >> 4);
        }
    }
    return c;
}

#else // no custom ISA

#ifdef LOAD_OPT
INLINE_OPTION int32_t dot_product_int16(const int16_t* a, const int16_t* b, const size_t len){
    int32_t c = 0;
    size_t len_s2 = (len >> 1) << 1;
    for (size_t k = 0; k < len_s2; k += 2) {
        int32_t a_slice = *(int32_t*)(a + k);
        int32_t b_slice = *(int32_t*)(b + k);
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
    size_t rem = len - len_s2;
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c += a[i] * b[i];
    }
    return c;
}

INLINE_OPTION int32_t dot_product_int8(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s4 = (len >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t a_slice = *(int32_t*)(a + k);
        int32_t b_slice = *(int32_t*)(b + k);
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
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c += a[i] * b[i];
    }
    return c;
}

INLINE_OPTION int32_t dot_product_int4(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_bytes = len >> 1; // len passed in as number of nibbles
    size_t len_s4 = ((len_bytes) >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t a_slice = *(int32_t*)(a + k);
        int32_t b_slice = *(int32_t*)(b + k);
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
        int8_t al, bl;
        for (size_t i = len_s4; i < len_bytes; i++) {
            al = a[i];
            bl = b[i];
            c += (al >> 4) * (bl >> 4);
            al <<= 4;
            bl <<= 4;
            c += (al >> 4) * (bl >> 4);
        }
    }
    return c;
}

INLINE_OPTION int32_t dot_product_int16_int8(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s4 = (len >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t b_slice = *(int32_t*)(b + k);
        for (size_t j = 0; j < 4; j += 2) {
            int32_t a_slice = *(int32_t*)(a + k + 2 - j); // MSB first
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
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c += a[i] * (int16_t)b[i];
    }
    return c;
}

INLINE_OPTION int32_t dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s8 = (len >> 3) << 3;
    for (size_t k = 0; k < len_s8; k += 8) {
        int32_t b_slice = *(int32_t*)(b + (k >> 1));
        for (size_t j = 0; j < 8; j += 2) {
            int32_t a_slice = *(int32_t*)(a + k + 6 - j); // MSB first
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
    size_t rem = len - len_s8;
    if (rem > 0) {
        int8_t bl;
        for (size_t i = len_s8; i < len; i += 2) {
            bl = b[i>>1];
            c += a[i+1] * (int16_t)(bl >> 4);
            bl <<= 4;
            c += a[i] * (int16_t)(bl >> 4);
        }
    }
    return c;
}

INLINE_OPTION int32_t dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s8 = (len >> 3) << 3;
    for (size_t k = 0; k < len_s8; k += 8) {
        int32_t b_slice = *(int32_t*)(b + (k >> 1));
        for (size_t j = 0; j < 8; j += 4) {
            int32_t a_slice = *(int32_t*)(a + k + 4 - j); // MSB first
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
    size_t rem = len - len_s8;
    if (rem > 0) {
        int8_t bl;
        for (size_t i = len_s8; i < len; i += 2) {
            bl = b[i>>1];
            c += a[i+1] * (int8_t)(bl >> 4);
            bl <<= 4;
            c += a[i] * (int8_t)(bl >> 4);
        }
    }
    return c;
}

#else // generic implementation
INLINE_OPTION int32_t dot_product_int16(const int16_t* a, const int16_t* b, const size_t len){
    int32_t c = 0;
    for (size_t k = 0; k < len; k++) c += a[k] * b[k];
    return c;
}

INLINE_OPTION int32_t dot_product_int8(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    for (size_t k = 0; k < len; k++) c += a[k] * b[k];
    return c;
}

INLINE_OPTION int32_t dot_product_int4(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    int8_t al, bl;
    size_t len_bytes = len >> 1; // len passed in as number of nibbles
    for (size_t k = 0; k < len_bytes; k++) {
        al = a[k];
        bl = b[k];
        c += (al >> 4) * (bl >> 4);
        al <<= 4;
        bl <<= 4;
        c += (al >> 4) * (bl >> 4);
    }
    return c;
}

INLINE_OPTION int32_t dot_product_int16_int8(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    for (size_t k = 0; k < len; k++) c += a[k] * (int16_t)b[k];
    return c;
}

INLINE_OPTION int32_t dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    int8_t bl;
    for (size_t k = 0; k < len; k += 2) {
        bl = b[k>>1];
        c += a[k+1] * (int16_t)(bl >> 4);
        bl <<= 4;
        c += a[k] * (int16_t)(bl >> 4);
    }
    return c;
}

INLINE_OPTION int32_t dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    int8_t bl;
    for (size_t k = 0; k < len; k += 2) {
        bl = b[k>>1];
        c += a[k+1] * (int8_t)(bl >> 4);
        bl <<= 4;
        c += a[k] * (int8_t)(bl >> 4);
    }
    return c;
}

#endif // LOAD_OPT
#endif // CUSTOM_ISA

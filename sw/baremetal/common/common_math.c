#include "common_math.h"

#ifdef PARTIAL_ZBB_SUPPORT
int32_t max(int32_t a, int32_t b) {
    int32_t c;
    asm volatile(
        ".insn r 0x33, 0x6, 0x5, %0, %1, %2"
        : "=r"(c)
        : "r"(a), "r"(b)
    );
    return c;
}

uint32_t maxu(uint32_t a, uint32_t b) {
    uint32_t c;
    asm volatile(
        ".insn r 0x33, 0x7, 0x5, %0, %1, %2"
        : "=r"(c)
        : "r"(a), "r"(b)
    );
    return c;
}

int32_t min(int32_t a, int32_t b) {
    int32_t c;
    asm volatile(
        ".insn r 0x33, 0x4, 0x5, %0, %1, %2"
        : "=r"(c)
        : "r"(a), "r"(b)
    );
    return c;
}

uint32_t minu(uint32_t a, uint32_t b) {
    uint32_t c;
    asm volatile(
        ".insn r 0x33, 0x5, 0x5, %0, %1, %2"
        : "=r"(c)
        : "r"(a), "r"(b)
    );
    return c;
}

#else
int32_t max(int32_t a, int32_t b) {
    return a > b ? a : b;
}

uint32_t maxu(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

int32_t min(int32_t a, int32_t b) {
    return a < b ? a : b;
}

uint32_t minu(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}
#endif

#ifdef CUSTOM_ISA
INLINE_OPTION
void _simd_add_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len) {
    size_t len_s2 = (len >> 1) << 1;
    int32_t c_slice;
    for (size_t k = 0; k < len_s2; k += 2) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
        c_slice = add16(a_slice, b_slice);
        *(int32_t*)(c + k) = c_slice;
    }
    size_t rem = len - len_s2;
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void _simd_add_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len) {
    size_t len_s4 = (len >> 2) << 2;
    int32_t c_slice;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
        c_slice = add8(a_slice, b_slice);
        *(int32_t*)(c + k) = c_slice;
    }
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void _simd_sub_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len) {
    size_t len_s2 = (len >> 1) << 1;
    int32_t c_slice;
    for (size_t k = 0; k < len_s2; k += 2) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
        c_slice = sub16(a_slice, b_slice);
        *(int32_t*)(c + k) = c_slice;
    }
    size_t rem = len - len_s2;
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void _simd_sub_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len) {
    size_t len_s4 = (len >> 2) << 2;
    int32_t c_slice;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
        c_slice = sub8(a_slice, b_slice);
        *(int32_t*)(c + k) = c_slice;
    }
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void _simd_mul_int16(
    const int16_t* a, const int16_t* b, int32_t* c, const size_t len) {
    size_t len_s2 = (len >> 1) << 1;
    for (size_t k = 0; k < len_s2; k += 2) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
        register int32_t c_slice_1 asm("t5");
        register int32_t c_slice_2 asm("t6");
        asm volatile(
            "mul16 %[c1], %[a], %[b];"
            : [c1] "=r" (c_slice_1), [c2] "=r" (c_slice_2)
            : [a] "r" (a_slice), [b] "r" (b_slice)
            :
        );
        *(c + k) = c_slice_1;
        *(c + k + 1) = c_slice_2;
    }
    size_t rem = len - len_s2;
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] * b[i];
    }
}

INLINE_OPTION
void _simd_mul_int8(
    const int8_t* a, const int8_t* b, int16_t* c, const size_t len) {
    size_t len_s4 = (len >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
        register int32_t c_slice_1 asm("t5");
        register int32_t c_slice_2 asm("t6");
        asm volatile(
            "mul8 %[c1], %[a], %[b];"
            : [c1] "=r" (c_slice_1), [c2] "=r" (c_slice_2)
            : [a] "r" (a_slice), [b] "r" (b_slice)
            :
        );
        *(int32_t*)(c + k) = c_slice_1;
        *(int32_t*)(c + k + 2) = c_slice_2;
    }
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] * b[i];
    }
}

INLINE_OPTION
void _simd_mul_uint16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len) {
    size_t len_s2 = (len >> 1) << 1;
    for (size_t k = 0; k < len_s2; k += 2) {
        const uint32_t a_slice = *(const uint32_t*)(a + k);
        const uint32_t b_slice = *(const uint32_t*)(b + k);
        register uint32_t c_slice_1 asm("t5");
        register uint32_t c_slice_2 asm("t6");
        asm volatile(
            "mul16u %[c1], %[a], %[b];"
            : [c1] "=r" (c_slice_1), [c2] "=r" (c_slice_2)
            : [a] "r" (a_slice), [b] "r" (b_slice)
            :
        );
        *(c + k) = c_slice_1;
        *(c + k + 1) = c_slice_2;
    }
    size_t rem = len - len_s2;
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] * b[i];
    }
}

INLINE_OPTION
void _simd_mul_uint8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len) {
    size_t len_s4 = (len >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        const uint32_t a_slice = *(const uint32_t*)(a + k);
        const uint32_t b_slice = *(const uint32_t*)(b + k);
        register uint32_t c_slice_1 asm("t5");
        register uint32_t c_slice_2 asm("t6");
        asm volatile(
            "mul8u %[c1], %[a], %[b];"
            : [c1] "=r" (c_slice_1), [c2] "=r" (c_slice_2)
            : [a] "r" (a_slice), [b] "r" (b_slice)
            :
        );
        *(uint32_t*)(c + k) = c_slice_1;
        *(uint32_t*)(c + k + 2) = c_slice_2;
    }
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] * b[i];
    }
}

INLINE_OPTION
int32_t _simd_dot_product_int16(
    const int16_t* a, const int16_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s2 = (len >> 1) << 1;
    for (size_t k = 0; k < len_s2; k += 2) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
        c += dot16(a_slice, b_slice);
    }
    size_t rem = len - len_s2;
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c += a[i] * b[i];
    }
    return c;
}

#ifdef STREAMING // optimized for pipeline utilization, 4x unrolling

INLINE_OPTION
int32_t _simd_dot_product_int8(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s16 = (len >> 4) << 4;
    for (size_t k = 0; k < len_s16; k += 16) {
        int32_t a_arr[4], b_arr[4], p_arr[4];

        #pragma GCC unroll 4
        for (size_t i = 0; i < 4; i++) {
            a_arr[i] = *(const int32_t*)(a + k + i * 4);
            b_arr[i] = *(const int32_t*)(b + k + i * 4);
        }
        asm volatile (
            "dot8 %[p1], %[a0], %[b0];"
            "dot8 %[p2], %[a1], %[b1];"
            "dot8 %[p3], %[a2], %[b2];"
            "dot8 %[p4], %[a3], %[b3];"
            "add %[c], %[c], %[p1];"
            "add %[c], %[c], %[p2];"
            "add %[c], %[c], %[p3];"
            "add %[c], %[c], %[p4];"
            : [c] "+r" (c),
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
            const int32_t a_slice = *(const int32_t*)(a + i);
            const int32_t b_slice = *(const int32_t*)(b + i);
            c += dot8(a_slice, b_slice);
        }
    }
    size_t rem = len - (len_s16 + len_s4);
    if (rem > 0) {
        for (size_t i = (len_s16 + len_s4); i < len; i++) c += a[i] * b[i];
    }
    return c;
}

#else // one SIMD inst at a time

INLINE_OPTION
int32_t _simd_dot_product_int8(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s4 = (len >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
        c += dot8(a_slice, b_slice);
    }
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c += a[i] * b[i];
    }
    return c;
}
#endif // STREAMING

INLINE_OPTION
int32_t _simd_dot_product_int4(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_bytes = len >> 1; // len passed in as number of nibbles
    size_t len_s4 = ((len_bytes) >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
        c += dot4(a_slice, b_slice);
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

INLINE_OPTION
int32_t _simd_dot_product_int16_int8(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s4 = (len >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        int32_t p1, p2;
        const int32_t b_slice = *(const int32_t*)(b + k);
        const int32_t a_slice_1 = *(const int32_t*)(a + k);
        const int32_t a_slice_2 = *(const int32_t*)(a + k + 2);
        asm (
            "unpk8 "TOSTR(RD_1)", %[b];"
            // x30 (RD_1) = 2 lower halves, x31 (RDP_1) = 2 upper halves
            "dot16 %[p1], %[a1], "TOSTR(RD_1)";"
            "dot16 %[p2], %[a2], "TOSTR(RDP_1)";"
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

INLINE_OPTION
int32_t _simd_dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s8 = (len >> 3) << 3;
    for (size_t k = 0; k < len_s8; k += 8) {
        int32_t p1, p2;
        const int32_t b_slice = *(const int32_t*)(b + (k >> 1));
        const int32_t a_slice_1 = *(const int32_t*)(a + k);
        const int32_t a_slice_2 = *(const int32_t*)(a + k + 2);
        const int32_t a_slice_3 = *(const int32_t*)(a + k + 4);
        const int32_t a_slice_4 = *(const int32_t*)(a + k + 6);
        asm (
            "unpk4 "TOSTR(RD_1)", %[b];" // nibbles to bytes
            "unpk8 "TOSTR(RD_2)", "TOSTR(RD_1)";" // low bytes to halfwords
            "dot16 %[p1], %[a1], "TOSTR(RD_2)";"
            "dot16 %[p2], %[a2], "TOSTR(RDP_2)";"
            "add %[c], %[c], %[p1];"
            "add %[c], %[c], %[p2];"
            "unpk8 "TOSTR(RD_2)", "TOSTR(RDP_1)";" // high bytes to halfwords
            "dot16 %[p1], %[a3], "TOSTR(RD_2)";"
            "dot16 %[p2], %[a4], "TOSTR(RDP_2)";"
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

INLINE_OPTION
int32_t _simd_dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s8 = (len >> 3) << 3;
    for (size_t k = 0; k < len_s8; k += 8) {
        int32_t p1, p2;
        const int32_t b_slice = *(const int32_t*)(b + (k >> 1));
        const int32_t a_slice_1 = *(const int32_t*)(a + k);
        const int32_t a_slice_2 = *(const int32_t*)(a + k + 4);
        asm (
            "unpk4 "TOSTR(RD_1)", %[b];"
            "dot8 %[p1], %[a1], "TOSTR(RD_1)";"
            "dot8 %[p2], %[a2], "TOSTR(RDP_1)";"
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

INLINE_OPTION
int32_t _simd_dot_product_int8_int2(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s16 = (len >> 4) << 4;
    for (size_t k = 0; k < len_s16; k += 16) {
        int32_t p1, p2;
        const int32_t b_slice = *(const int32_t*)(b + (k >> 2));
        const int32_t a_slice_1 = *(const int32_t*)(a + k);
        const int32_t a_slice_2 = *(const int32_t*)(a + k + 4);
        const int32_t a_slice_3 = *(const int32_t*)(a + k + 8);
        const int32_t a_slice_4 = *(const int32_t*)(a + k + 12);
        asm (
            "unpk2 "TOSTR(RD_1)", %[b];" // crumbs to nibbles
            "unpk4 "TOSTR(RD_2)", "TOSTR(RD_1)";" // low nibbles to bytes
            "dot8 %[p1], %[a1], "TOSTR(RD_2)";"
            "dot8 %[p2], %[a2], "TOSTR(RDP_2)";"
            "add %[c], %[c], %[p1];"
            "add %[c], %[c], %[p2];"
            "unpk4 "TOSTR(RD_2)", "TOSTR(RDP_1)";" // high nibbles to bytes
            "dot8 %[p1], %[a3], "TOSTR(RD_2)";"
            "dot8 %[p2], %[a4], "TOSTR(RDP_2)";"
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
    size_t rem = len - len_s16;
    if (rem > 0) {
        int8_t bl;
        for (size_t i = len_s16; i < len; i += 4) {
            bl = b[i>>2];
            c += a[i+3] * (int8_t)(bl >> 6);
            bl <<= 2;
            c += a[i+2] * (int8_t)(bl >> 6);
            bl <<= 2;
            c += a[i+1] * (int8_t)(bl >> 6);
            bl <<= 2;
            c += a[i] * (int8_t)(bl >> 6);
        }
    }
    return c;
}

#else // no custom ISA

#ifdef LOAD_OPT
INLINE_OPTION
void add_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len) {
    size_t len_s2 = (len >> 1) << 1;
    int32_t c_slice;
    for (size_t k = 0; k < len_s2; k += 2) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
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
    size_t rem = len - len_s2;
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void add_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len) {
    // FIXME: doesn't yield better performance than generic implementation
    size_t len_s4 = (len >> 2) << 2;
    int32_t c_slice;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
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
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c += a[i] + b[i];
    }
}

INLINE_OPTION
void sub_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len) {
    size_t len_s2 = (len >> 1) << 1;
    int32_t c_slice;
    for (size_t k = 0; k < len_s2; k += 2) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
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
    size_t rem = len - len_s2;
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] - b[i];
    }
}

INLINE_OPTION
void sub_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len) {
    // FIXME: doesn't yield better performance than generic implementation
    size_t len_s4 = (len >> 2) << 2;
    int32_t c_slice;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
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
    size_t rem = len - len_s4;
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c += a[i] - b[i];
    }
}

INLINE_OPTION
int32_t dot_product_int16(const int16_t* a, const int16_t* b, const size_t len){
    int32_t c = 0;
    size_t len_s2 = (len >> 1) << 1;
    for (size_t k = 0; k < len_s2; k += 2) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
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

INLINE_OPTION
int32_t dot_product_int8(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s4 = (len >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
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

INLINE_OPTION
int32_t dot_product_int4(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_bytes = len >> 1; // len passed in as number of nibbles
    size_t len_s4 = ((len_bytes) >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int32_t a_slice = *(const int32_t*)(a + k);
        const int32_t b_slice = *(const int32_t*)(b + k);
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

INLINE_OPTION
int32_t dot_product_int16_int8(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s4 = (len >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int32_t b_slice = *(const int32_t*)(b + k);
        for (size_t j = 0; j < 4; j += 2) {
            const int32_t a_slice = *(const int32_t*)(a + k + 2 - j);//MSB first
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

INLINE_OPTION
int32_t dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s8 = (len >> 3) << 3;
    for (size_t k = 0; k < len_s8; k += 8) {
        const int32_t b_slice = *(const int32_t*)(b + (k >> 1));
        for (size_t j = 0; j < 8; j += 2) {
            const int32_t a_slice = *(const int32_t*)(a + k + 6 - j);
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

INLINE_OPTION
int32_t dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s8 = (len >> 3) << 3;
    for (size_t k = 0; k < len_s8; k += 8) {
        const int32_t b_slice = *(const int32_t*)(b + (k >> 1));
        for (size_t j = 0; j < 8; j += 4) {
            const int32_t a_slice = *(const int32_t*)(a + k + 4 - j);
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

INLINE_OPTION
int32_t dot_product_int8_int2(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s16 = (len >> 4) << 4;
    for (size_t k = 0; k < len_s16; k += 16) {
        const int32_t b_slice = *(const int32_t*)(b + (k >> 2));
        for (size_t j = 0; j < 16; j += 4) {
            const int32_t a_slice = *(const int32_t*)(a + k + 12 - j);
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
    size_t rem = len - len_s16;
    if (rem > 0) {
        int8_t bl;
        for (size_t i = len_s16; i < len; i += 4) {
            bl = b[i>>2];
            c += a[i+3] * (int8_t)(bl >> 6);
            bl <<= 2;
            c += a[i+2] * (int8_t)(bl >> 6);
            bl <<= 2;
            c += a[i+1] * (int8_t)(bl >> 6);
            bl <<= 2;
            c += a[i] * (int8_t)(bl >> 6);
        }
    }
    return c;
}

#else // generic implementation
INLINE_OPTION
void add_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len) {
    for (size_t k = 0; k < len; k++) c[k] = a[k] + b[k];
}

INLINE_OPTION
void add_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len) {
    for (size_t k = 0; k < len; k++) c[k] = a[k] + b[k];
}

INLINE_OPTION
void sub_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len) {
    for (size_t k = 0; k < len; k++) c[k] = a[k] - b[k];
}

INLINE_OPTION
void sub_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len) {
    for (size_t k = 0; k < len; k++) c[k] = a[k] - b[k];
}

INLINE_OPTION
void mul_int16(
    const int16_t* a, const int16_t* b, int32_t* c, const size_t len) {
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
}

INLINE_OPTION
void mul_int8(
    const int8_t* a, const int8_t* b, int16_t* c, const size_t len) {
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
}

INLINE_OPTION
void mul_uint16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len) {
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
}

INLINE_OPTION
void mul_uint8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len) {
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
}

INLINE_OPTION
int32_t dot_product_int16(const int16_t* a, const int16_t* b, const size_t len){
    int32_t c = 0;
    for (size_t k = 0; k < len; k++) c += a[k] * b[k];
    return c;
}

INLINE_OPTION
int32_t dot_product_int8(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    for (size_t k = 0; k < len; k++) c += a[k] * b[k];
    return c;
}

INLINE_OPTION
int32_t dot_product_int4(const int8_t* a, const int8_t* b, const size_t len) {
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

INLINE_OPTION
int32_t dot_product_int16_int8(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    for (size_t k = 0; k < len; k++) c += a[k] * (int16_t)b[k];
    return c;
}

INLINE_OPTION
int32_t dot_product_int16_int4(
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

INLINE_OPTION
int32_t dot_product_int8_int4(
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

INLINE_OPTION
int32_t dot_product_int8_int2(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    int8_t bl;
    for (size_t k = 0; k < len; k += 4) {
        bl = b[k>>2];
        c += a[k+3] * (int8_t)(bl >> 6);
        bl <<= 2;
        c += a[k+2] * (int8_t)(bl >> 6);
        bl <<= 2;
        c += a[k+1] * (int8_t)(bl >> 6);
        bl <<= 2;
        c += a[k] * (int8_t)(bl >> 6);
    }
    return c;
}

#endif // LOAD_OPT
#endif // CUSTOM_ISA

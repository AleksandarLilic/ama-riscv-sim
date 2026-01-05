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
    size_t len_s2 = ((len >> 1) << 1);
    int16x2_t c_slice;
    for (size_t k = 0; k < len_s2; k += 2) {
        const int16x2_t a_slice = v_load_int16x2(a + k);
        const int16x2_t b_slice = v_load_int16x2(b + k);
        c_slice = _add16(a_slice, b_slice);
        v_store_int16x2(c + k, c_slice);
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void _simd_add_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len) {
    size_t len_s4 = ((len >> 2) << 2);
    int8x4_t c_slice;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int8x4_t a_slice = v_load_int8x4(a + k);
        const int8x4_t b_slice = v_load_int8x4(b + k);
        c_slice = _add8(a_slice, b_slice);
        v_store_int8x4(c + k, c_slice);
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void _simd_sub_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len) {
    size_t len_s2 = ((len >> 1) << 1);
    int16x2_t c_slice;
    for (size_t k = 0; k < len_s2; k += 2) {
        const int16x2_t a_slice = v_load_int16x2(a + k);
        const int16x2_t b_slice = v_load_int16x2(b + k);
        c_slice = _sub16(a_slice, b_slice);
        v_store_int16x2(c + k, c_slice);
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void _simd_sub_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len) {
    size_t len_s4 = ((len >> 2) << 2);
    int8x4_t c_slice;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int8x4_t a_slice = v_load_int8x4(a + k);
        const int8x4_t b_slice = v_load_int8x4(b + k);
        c_slice = _sub8(a_slice, b_slice);
        v_store_int8x4(c + k, c_slice);
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] + b[i];
    }
}

INLINE_OPTION
void _simd_mul_int16(
    const int16_t* a, const int16_t* b, int32_t* c, const size_t len) {
    size_t len_s2 = ((len >> 1) << 1);
    for (size_t k = 0; k < len_s2; k += 2) {
        const int16x2_t a_slice = v_load_int16x2(a + k);
        const int16x2_t b_slice = v_load_int16x2(b + k);
        int32x2_t c_slice = _wmul16(a_slice, b_slice);
        *(c + k) = c_slice.w.lo;
        *(c + k + 1) = c_slice.w.hi;
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] * b[i];
    }
}

INLINE_OPTION
void _simd_mul_uint16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len) {
    size_t len_s2 = ((len >> 1) << 1);
    for (size_t k = 0; k < len_s2; k += 2) {
        const uint16x2_t a_slice = v_load_uint16x2(a + k);
        const uint16x2_t b_slice = v_load_uint16x2(b + k);
        uint32x2_t c_slice = _wmul16u(a_slice, b_slice);
        *(c + k) = c_slice.w.lo;
        *(c + k + 1) = c_slice.w.hi;
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] * b[i];
    }
}

INLINE_OPTION
void _simd_mul_int8(
    const int8_t* a, const int8_t* b, int16_t* c, const size_t len) {
    size_t len_s4 = ((len >> 2) << 2);
    for (size_t k = 0; k < len_s4; k += 4) {
        const int8x4_t a_slice = v_load_int8x4(a + k);
        const int8x4_t b_slice = v_load_int8x4(b + k);
        int16x4_t c_slice = _wmul8(a_slice, b_slice);
        v_store_int16x2(c + k, c_slice.w.lo);
        v_store_int16x2(c + k + 2, c_slice.w.hi);
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] * b[i];
    }
}

INLINE_OPTION
void _simd_mul_uint8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len) {
    size_t len_s4 = ((len >> 2) << 2);
    for (size_t k = 0; k < len_s4; k += 4) {
        const uint8x4_t a_slice = v_load_uint8x4(a + k);
        const uint8x4_t b_slice = v_load_uint8x4(b + k);
        uint16x4_t c_slice = _wmul8u(a_slice, b_slice);
        v_store_uint16x2(c + k, c_slice.w.lo);
        v_store_uint16x2(c + k + 2, c_slice.w.hi);
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] * b[i];
    }
}

INLINE_OPTION
int32_t _simd_dot_product_int16(
    const int16_t* a, const int16_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s2 = ((len >> 1) << 1);
    for (size_t k = 0; k < len_s2; k += 2) {
        const int16x2_t a_slice = v_load_int16x2(a + k);
        const int16x2_t b_slice = v_load_int16x2(b + k);
        _dot16(a_slice, b_slice, &c);
    }
    size_t rem = (len - len_s2);
    if (rem > 0) {
        for (size_t i = len_s2; i < len; i++) c += a[i] * b[i];
    }
    return c;
}

#ifdef SIMD_UNROLL
INLINE_OPTION
int32_t _simd_dot_product_int8(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = 3; // unroll degree
    static const size_t deg = (2 + udeg); // +2 for bytes to words
    size_t tile = ((len >> deg) << deg); // +2 to words, +3 for 8x unroll

    for (size_t k = 0; k < tile; k += (1 << deg)) {
        int8x4_t a_arr[8], b_arr[8]; // 8=(1<<udeg), but compiler is not happy

        #pragma GCC unroll 8
        for (size_t i = 0; i < 8; i++) {
            a_arr[i] = v_load_int8x4(a + k + i * 4);
            b_arr[i] = v_load_int8x4(b + k + i * 4);
        }
        asm volatile (
            "dot8 %[c], %[a1], %[b1]\n\t"
            "dot8 %[c], %[a2], %[b2]\n\t"
            "dot8 %[c], %[a3], %[b3]\n\t"
            "dot8 %[c], %[a0], %[b0]\n\t" // scheduling, b0 often loaded last
            : [c] "+r" (c)
            : [a0] "r" (a_arr[0]), [b0] "r" (b_arr[0]),
              [a1] "r" (a_arr[1]), [b1] "r" (b_arr[1]),
              [a2] "r" (a_arr[2]), [b2] "r" (b_arr[2]),
              [a3] "r" (a_arr[3]), [b3] "r" (b_arr[3])
            :
        );
        // let compiler schedule loads in between to reduce rf pressure
        asm volatile (
            "dot8 %[c], %[a5], %[b5]\n\t"
            "dot8 %[c], %[a6], %[b6]\n\t"
            "dot8 %[c], %[a7], %[b7]\n\t"
            "dot8 %[c], %[a4], %[b4]\n\t"
            : [c] "+r" (c)
            : [a4] "r" (a_arr[4]), [b4] "r" (b_arr[4]),
              [a5] "r" (a_arr[5]), [b5] "r" (b_arr[5]),
              [a6] "r" (a_arr[6]), [b6] "r" (b_arr[6]),
              [a7] "r" (a_arr[7]), [b7] "r" (b_arr[7])
            :
        );
    }

    // large tiles exhausted, do 4 bytes instead, if any
    size_t tile_single = (((len - tile) >> 2) << 2);
    if (tile_single > 0) {
        size_t start = tile;
        size_t end = (tile + tile_single);
        c += _simd_dot_product_int8_core((a + start), (b + start), end);
    }
    return c;
}

#else
INLINE_OPTION
int32_t _simd_dot_product_int8(
    const int8_t* a, const int8_t* b, const size_t len) {
    return _simd_dot_product_int8_core(a, b, len);
}

#endif // SIMD_UNROLL

INLINE_OPTION
int32_t _simd_dot_product_int8_core(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s4 = ((len >> 2) << 2);
    for (size_t k = 0; k < len_s4; k += 4) {
        const int8x4_t a_slice = v_load_int8x4(a + k);
        const int8x4_t b_slice = v_load_int8x4(b + k);
        _dot8(a_slice, b_slice, &c);
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c += a[i] * b[i];
    }
    return c;
}

INLINE_OPTION
int32_t _simd_dot_product_int4(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_bytes = (len >> 1); // len passed in as number of nibbles
    size_t len_s4 = ((len_bytes) >> 2) << 2;
    for (size_t k = 0; k < len_s4; k += 4) {
        const int4x8_t a_slice = v_load_int4x8(a + k);
        const int4x8_t b_slice = v_load_int4x8(b + k);
        _dot4(a_slice, b_slice, &c);
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
    size_t len_s4 = ((len >> 2) << 2);
    for (size_t k = 0; k < len_s4; k += 4) {
        const int8x4_t b_slice = v_load_int8x4(b + k);
        const int16x2_t a_slice_1 = v_load_int16x2(a + k);
        const int16x2_t a_slice_2 = v_load_int16x2(a + k + 2);
        int16x4_t b_slice_wide;
        asm volatile (
            "widen8 %[bw], %[b]\n\t"
            // bw.w.lo = 2 lower halves, bw.w.hi = 2 upper halves
            : [bw] "=r" (b_slice_wide.d)
            : [b] "r" (b_slice)
        );
        asm volatile (
            "dot16 %[c], %[a1], %[bw_lo]\n\t"
            "dot16 %[c], %[a2], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide.w.lo), [bw_hi] "r" (b_slice_wide.w.hi),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
        );
    }
    size_t rem = (len - len_s4);
    if (rem > 0) {
        for (size_t i = len_s4; i < len; i++) c += a[i] * (int16_t)b[i];
    }
    return c;
}

INLINE_OPTION
int32_t _simd_dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s8 = ((len >> 3) << 3);
    for (size_t k = 0; k < len_s8; k += 8) {
        const int4x8_t b_slice = v_load_int4x8(b + (k >> 1));
        const int16x2_t a_slice_1 = v_load_int16x2(a + k);
        const int16x2_t a_slice_2 = v_load_int16x2(a + k + 2);
        const int16x2_t a_slice_3 = v_load_int16x2(a + k + 4);
        const int16x2_t a_slice_4 = v_load_int16x2(a + k + 6);
        int8x8_t b_slice_wide_b;
        int16x4_t b_slice_wide_h;
        asm volatile (
            "widen4 %[bw], %[b]\n\t"
            : [bw] "=r" (b_slice_wide_b.d)
            : [b] "r" (b_slice) // nibbles to bytes
        );
        asm volatile (
            "widen8 %[bw], %[b]\n\t"
            : [bw] "=r" (b_slice_wide_h.d)
            : [b] "r" (b_slice_wide_b.w.lo) // low bytes to halfwords
        );
        asm volatile (
            "dot16 %[c], %[a1], %[bw_lo]\n\t"
            "dot16 %[c], %[a2], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide_h.w.lo),
              [bw_hi] "r" (b_slice_wide_h.w.hi),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
        );
        asm volatile (
            "widen8 %[bw], %[b]\n\t"
            : [bw] "=r" (b_slice_wide_h.d)
            : [b] "r" (b_slice_wide_b.w.hi) // high bytes to halfwords
        );
        asm volatile (
            "dot16 %[c], %[a3], %[bw_lo]\n\t"
            "dot16 %[c], %[a4], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide_h.w.lo),
              [bw_hi] "r" (b_slice_wide_h.w.hi),
              [a3] "r" (a_slice_3), [a4] "r" (a_slice_4)
        );
    }
    size_t rem = (len - len_s8);
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

#ifdef SIMD_UNROLL
INLINE_OPTION
int32_t _simd_dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = 3; // unroll degree
    static const size_t deg = (2 + 1 + udeg); // +2 for bytes to words, +1 widen
    size_t tile = ((len >> deg) << deg);

    for (size_t k = 0; k < tile; k += (1 << deg)) {
        int8x4_t a_slice_1, a_slice_2;
        int4x8_t b_slice;
        int8x8_t b_slice_wide;

        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {                  // 0,  1,  2,  3
            b_slice = v_load_int4x8(b + ((k + i * 8) >> 1)); // 0,  4,  8, 12
            a_slice_1 = v_load_int8x4(a + k     + i * 8);    // 0,  8, 16, 24
            a_slice_2 = v_load_int8x4(a + k + 4 + i * 8);    // 4, 12, 20, 28

            asm volatile (
                "widen4 %[bw], %[b]\n\t"
                : [bw] "=r" (b_slice_wide.d)
                : [b] "r" (b_slice)
            );
            asm volatile (
                "dot8 %[c], %[a1], %[bw_lo]\n\t"
                "dot8 %[c], %[a2], %[bw_hi]\n\t"
                : [c] "+r" (c)
                : [bw_lo] "r" (b_slice_wide.w.lo),
                  [bw_hi] "r" (b_slice_wide.w.hi),
                  [a1] "r" (a_slice_1),
                  [a2] "r" (a_slice_2)
            );
        }
    }

    // large tiles exhausted, do 4 bytes instead, if any
    size_t tile_single = (((len - tile) >> 3) << 3);
    if (tile_single > 0) {
        size_t start = tile;
        size_t end = (tile + tile_single);
        c += _simd_dot_product_int8_int4_core((a + start), (b + start), end);
    }
    return c;
}

#else
INLINE_OPTION
int32_t _simd_dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len) {
    return _simd_dot_product_int8_int4_core(a, b, len);
}

#endif // SIMD_UNROLL

INLINE_OPTION
int32_t _simd_dot_product_int8_int4_core(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s8 = ((len >> 3) << 3);
    for (size_t k = 0; k < len_s8; k += 8) {
        const int4x8_t b_slice = v_load_int4x8(b + (k >> 1));
        const int8x4_t a_slice_1 = v_load_int8x4(a + k);
        const int8x4_t a_slice_2 = v_load_int8x4(a + k + 4);
        int8x8_t b_slice_wide;
        asm volatile (
            "widen4 %[bw], %[b]\n\t"
            : [bw] "=r" (b_slice_wide.d)
            : [b] "r" (b_slice)
        );
        asm volatile (
            "dot8 %[c], %[a1], %[bw_lo]\n\t"
            "dot8 %[c], %[a2], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide.w.lo), [bw_hi] "r" (b_slice_wide.w.hi),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
        );
    }
    size_t rem = (len - len_s8);
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

#ifdef SIMD_UNROLL
INLINE_OPTION
int32_t _simd_dot_product_int8_int2(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = 2; // unroll degree
    static const size_t deg = (2 + 2 + udeg); // +2 for bytes to words, +2 widen
    size_t tile = ((len >> deg) << deg);

    for (size_t k = 0; k < tile; k += (1 << deg)) {
        int8x4_t a_slice_1, a_slice_2, a_slice_3, a_slice_4;
        int2x16_t b_slice;
        int4x16_t b_slice_wide_n;
        int8x8_t b_slice_wide_b;

        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {                    // 0,   1,  2
            b_slice = v_load_int2x16(b + ((k + i * 16) >> 2)); // 0,   4,  8
            a_slice_1 = v_load_int8x4(a + k      + i * 16);    // 0,  16, 32
            a_slice_2 = v_load_int8x4(a + k +  4 + i * 16);    // 4,  20, 36
            a_slice_3 = v_load_int8x4(a + k +  8 + i * 16);    // 8,  24, 40
            a_slice_4 = v_load_int8x4(a + k + 12 + i * 16);    // 12, 28, 44

            asm volatile (
                "widen2 %[bw], %[b]\n\t"
                : [bw] "=r" (b_slice_wide_n.d)
                : [b] "r" (b_slice) // crumbs to nibbles
            );
            asm volatile (
                "widen4 %[bw], %[b]\n\t"
                : [bw] "=r" (b_slice_wide_b.d)
                : [b] "r" (b_slice_wide_n.w.lo) // low nibbles to bytes
            );
            asm volatile (
                "dot8 %[c], %[a1], %[bw_lo]\n\t"
                "dot8 %[c], %[a2], %[bw_hi]\n\t"
                : [c] "+r" (c)
                : [bw_lo] "r" (b_slice_wide_b.w.lo),
                  [bw_hi] "r" (b_slice_wide_b.w.hi),
                  [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
            );
            asm volatile (
                "widen4 %[bw], %[b]\n\t"
                : [bw] "=r" (b_slice_wide_b.d)
                : [b] "r" (b_slice_wide_n.w.hi) // high nibbles to bytes
            );
            asm volatile (
                "dot8 %[c], %[a3], %[bw_lo]\n\t"
                "dot8 %[c], %[a4], %[bw_hi]\n\t"
                : [c] "+r" (c)
                : [bw_lo] "r" (b_slice_wide_b.w.lo),
                  [bw_hi] "r" (b_slice_wide_b.w.hi),
                  [a3] "r" (a_slice_3), [a4] "r" (a_slice_4)
            );
        }
    }

    // large tiles exhausted, do 4 bytes instead, if any
    size_t tile_single = (((len - tile) >> 3) << 3);
    if (tile_single > 0) {
        size_t start = tile;
        size_t end = (tile + tile_single);
        c += _simd_dot_product_int8_int2_core((a + start), (b + start), end);
    }
    return c;
}

#else
INLINE_OPTION
int32_t _simd_dot_product_int8_int2(
    const int8_t* a, const int8_t* b, const size_t len) {
    return _simd_dot_product_int8_int2_core(a, b, len);
}

#endif // SIMD_UNROLL

INLINE_OPTION
int32_t _simd_dot_product_int8_int2_core(
    const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    size_t len_s16 = ((len >> 4) << 4);
    for (size_t k = 0; k < len_s16; k += 16) {
        const int2x16_t b_slice = v_load_int2x16(b + (k >> 2));
        const int8x4_t a_slice_1 = v_load_int8x4(a + k);
        const int8x4_t a_slice_2 = v_load_int8x4(a + k + 4);
        const int8x4_t a_slice_3 = v_load_int8x4(a + k + 8);
        const int8x4_t a_slice_4 = v_load_int8x4(a + k + 12);
        int4x16_t b_slice_wide_n;
        int8x8_t b_slice_wide_b;
        asm volatile (
            "widen2 %[bw], %[b]\n\t"
            : [bw] "=r" (b_slice_wide_n.d)
            : [b] "r" (b_slice) // crumbs to nibbles
        );
        asm volatile (
            "widen4 %[bw], %[b]\n\t"
            : [bw] "=r" (b_slice_wide_b.d)
            : [b] "r" (b_slice_wide_n.w.lo) // low nibbles to bytes
        );
        asm volatile (
            "dot8 %[c], %[a1], %[bw_lo]\n\t"
            "dot8 %[c], %[a2], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide_b.w.lo),
              [bw_hi] "r" (b_slice_wide_b.w.hi),
              [a1] "r" (a_slice_1), [a2] "r" (a_slice_2)
        );
        asm volatile (
            "widen4 %[bw], %[b]\n\t"
            : [bw] "=r" (b_slice_wide_b.d)
            : [b] "r" (b_slice_wide_n.w.hi) // high nibbles to bytes
        );
        asm volatile (
            "dot8 %[c], %[a3], %[bw_lo]\n\t"
            "dot8 %[c], %[a4], %[bw_hi]\n\t"
            : [c] "+r" (c)
            : [bw_lo] "r" (b_slice_wide_b.w.lo),
              [bw_hi] "r" (b_slice_wide_b.w.hi),
              [a3] "r" (a_slice_3), [a4] "r" (a_slice_4)
        );
    }
    size_t rem = (len - len_s16);
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
void add_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len) {
    // FIXME: doesn't yield better performance than generic implementation
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
        for (size_t i = len_s4; i < len; i++) c += a[i] + b[i];
    }
}

INLINE_OPTION
void sub_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len) {
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
void sub_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len) {
    // FIXME: doesn't yield better performance than generic implementation
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
        for (size_t i = len_s4; i < len; i++) c += a[i] - b[i];
    }
}

INLINE_OPTION
int32_t dot_product_int16(const int16_t* a, const int16_t* b, const size_t len){
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
        for (size_t i = len_s2; i < len; i++) c += a[i] * b[i];
    }
    return c;
}

INLINE_OPTION
int32_t dot_product_int8(const int8_t* a, const int8_t* b, const size_t len) {
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
        for (size_t i = len_s4; i < len; i++) c += a[i] * b[i];
    }
    return c;
}

INLINE_OPTION
int32_t dot_product_int4(const int8_t* a, const int8_t* b, const size_t len) {
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
        for (size_t i = len_s4; i < len; i++) c += a[i] * (int16_t)b[i];
    }
    return c;
}

INLINE_OPTION
int32_t dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len) {
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

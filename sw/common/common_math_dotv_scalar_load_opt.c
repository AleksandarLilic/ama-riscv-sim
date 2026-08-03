
#include "common_math.h"
#include "common_math_dotv_scalar_core.h"

#if !defined(__riscv_xsimd) && defined(LOAD_OPT)

#ifdef M_UNROLL
#define LO_UNROLL 1
#else
#define LO_UNROLL 0
#endif

#define MAC_ITER_MIXED(ew_a, ty_a, ew_b, ty_b) \
    { \
        ty_a _a = (a_slice >> (32 - ew_a)); \
        ty_b _b = (b_slice >> (32 - ew_b)); \
        c += _a * (ty_a)_b; \
        a_slice <<= ew_a; \
        b_slice <<= ew_b; \
    }

#define MAC_ITER(ew, ty) \
    MAC_ITER_MIXED(ew, ty, ew, ty)

#define MAC_ITER_2(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty)

#define MAC_ITER_4(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty)

#define MAC_ITER_8(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty) \
    MAC_ITER(ew, ty)

#define MAC_ITER_16(ew, ty) \
    MAC_ITER_8(ew, ty) \
    MAC_ITER_8(ew, ty)

#define MAC_ITER_MIXED_2(ew_a, ty_a, ew_b, ty_b) \
    MAC_ITER_MIXED(ew_a, ty_a, ew_b, ty_b) \
    MAC_ITER_MIXED(ew_a, ty_a, ew_b, ty_b)

#define MAC_ITER_MIXED_4(ew_a, ty_a, ew_b, ty_b) \
    MAC_ITER_MIXED_2(ew_a, ty_a, ew_b, ty_b) \
    MAC_ITER_MIXED_2(ew_a, ty_a, ew_b, ty_b)

#define MAC_ITER_MIXED_8(ew_a, ty_a, ew_b, ty_b) \
    MAC_ITER_MIXED_4(ew_a, ty_a, ew_b, ty_b) \
    MAC_ITER_MIXED_4(ew_a, ty_a, ew_b, ty_b)

#define LOAD_SLICES(off) \
    a_slice = *(const int32_t*)((a + k + off)); \
    b_slice = *(const int32_t*)((b + k + off));

// mixed types consume the two operands at different rates
#define LOAD_A(off) a_slice = *(const int32_t*)((ap + (off)));
#define LOAD_B(off) b_slice = *(const int32_t*)((bp + (off)));

// tiles are sized to ~32 MACs:
// at ~6 instructions per MAC that is a ~200 instruction body

INLINE_OPTION
int32_t m_dotv_i16_i16(const int16_t* a, const int16_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = LO_UNROLL ? 3 : 0; // unroll degree
    static const size_t deg = (1 + udeg); // +1 for halfwords to words
    size_t tile = ((len >> deg) << deg); // +1 to words, +3 for 8x unroll
    const size_t p_inc = (1 << deg); // pointer increment

    for (size_t k = 0; k < tile; k += p_inc) {
        int32_t a_slice, b_slice;
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            LOAD_SLICES(i*2)
            MAC_ITER_2(16, int16_t)
        }
    }

    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i16_i16_scalar_core(a + tile, b + tile, rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i8_i8(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = LO_UNROLL ? 3 : 0; // unroll degree
    static const size_t deg = (2 + udeg); // +2 for bytes to words
    size_t tile = ((len >> deg) << deg); // +1 to words, +3 for 8x unroll
    const size_t p_inc = (1 << deg); // pointer increment

    for (size_t k = 0; k < tile; k += p_inc) {
        int32_t a_slice, b_slice;
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            LOAD_SLICES(i*4)
            MAC_ITER_4(8, int8_t)
        }
    }

    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i8_i8_scalar_core(a + tile, b + tile, rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i4_i4(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = LO_UNROLL ? 2 : 0; // unroll degree
    static const size_t deg = (2 + udeg); // +2 for bytes to words
    const size_t len_bytes = (len >> 1); // len passed in as number of nibbles
    const size_t tile = ((len_bytes >> deg) << deg); // 'k' is in bytes
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        int32_t a_slice, b_slice;
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            LOAD_SLICES(i*4)
            MAC_ITER_8(4, int8_t)
        }
    }

    size_t rem = (len_bytes - tile);
    if (rem > 0) {
        c += m_dotv_i4_i4_scalar_core(a + tile, b + tile, rem << 1);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i2_i2(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = LO_UNROLL ? 1 : 0; // unroll degree
    static const size_t deg = (2 + udeg); // +2 for bytes to words
    const size_t len_bytes = (len >> 2); // len passed in as number of crumbs
    const size_t tile = ((len_bytes >> deg) << deg); // 'k' is in bytes
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        int32_t a_slice, b_slice;
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            LOAD_SLICES(i*4)
            MAC_ITER_16(2, int8_t)
        }
    }

    size_t rem = (len_bytes - tile);
    if (rem > 0) {
        c += m_dotv_i2_i2_scalar_core(a + tile, b + tile, rem << 2);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i8(const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = LO_UNROLL ? 3 : 0; // unroll degree
    static const size_t deg = (2 + udeg); // +2 for el per 'b' word
    const size_t tile = ((len >> deg) << deg);
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        int32_t a_slice, b_slice;
        const int16_t* ap = a + k;
        const int8_t* bp = b + k;
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            LOAD_B(i*4)
            for (size_t j = 0; j < 4; j += 2) { // 'a' words, MSB first
                LOAD_A(i*4 + 2 - j)
                MAC_ITER_MIXED_2(16, int16_t, 8, int8_t)
            }
        }
    }

    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i16_i8_scalar_core(a + tile, b + tile, rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i4(const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = LO_UNROLL ? 2 : 0; // unroll degree
    static const size_t deg = (3 + udeg); // +3 for el per 'b' word
    const size_t tile = ((len >> deg) << deg);
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        int32_t a_slice, b_slice;
        const int16_t* ap = a + k;
        const int8_t* bp = b + (k >> 1);
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            LOAD_B(i*4)
            for (size_t j = 0; j < 8; j += 2) { // 'a' words, MSB first
                LOAD_A(i*8 + 6 - j)
                MAC_ITER_MIXED_2(16, int16_t, 4, int8_t)
            }
        }
    }

    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i16_i4_scalar_core(a + tile, b + (tile >> 1), rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i16_i2(const int16_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = LO_UNROLL ? 1 : 0; // unroll degree
    static const size_t deg = (4 + udeg); // +4 for el per 'b' word
    const size_t tile = ((len >> deg) << deg);
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        int32_t a_slice, b_slice;
        const int16_t* ap = a + k;
        const int8_t* bp = b + (k >> 2);
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            LOAD_B(i*4)
            for (size_t j = 0; j < 16; j += 2) { // 'a' words, MSB first
                LOAD_A(i*16 + 14 - j)
                MAC_ITER_MIXED_2(16, int16_t, 2, int8_t)
            }
        }
    }

    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i16_i2_scalar_core(a + tile, b + (tile >> 2), rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i8_i4(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = LO_UNROLL ? 2 : 0; // unroll degree
    static const size_t deg = (3 + udeg); // +3 for el per 'b' word
    const size_t tile = ((len >> deg) << deg);
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        int32_t a_slice, b_slice;
        const int8_t* ap = a + k;
        const int8_t* bp = b + (k >> 1);
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            LOAD_B(i*4)
            for (size_t j = 0; j < 8; j += 4) { // 'a' words, MSB first
                LOAD_A(i*8 + 4 - j)
                MAC_ITER_MIXED_4(8, int8_t, 4, int8_t)
            }
        }
    }

    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i8_i4_scalar_core(a + tile, b + (tile >> 1), rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i8_i2(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = LO_UNROLL ? 1 : 0; // unroll degree
    static const size_t deg = (4 + udeg); // +4 for el per 'b' word
    const size_t tile = ((len >> deg) << deg);
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        int32_t a_slice, b_slice;
        const int8_t* ap = a + k;
        const int8_t* bp = b + (k >> 2);
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            LOAD_B(i*4)
            for (size_t j = 0; j < 16; j += 4) { // 'a' words, MSB first
                LOAD_A(i*16 + 12 - j)
                MAC_ITER_MIXED_4(8, int8_t, 2, int8_t)
            }
        }
    }

    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i8_i2_scalar_core(a + tile, b + (tile >> 2), rem);
    }
    return c;
}

INLINE_OPTION
int32_t m_dotv_i4_i2(const int8_t* a, const int8_t* b, const size_t len) {
    int32_t c = 0;
    static const size_t udeg = LO_UNROLL ? 1 : 0; // unroll degree
    static const size_t deg = (4 + udeg); // +4 for el per 'b' word
    const size_t tile = ((len >> deg) << deg);
    const size_t p_inc = (1 << deg);

    for (size_t k = 0; k < tile; k += p_inc) {
        int32_t a_slice, b_slice;
        const int8_t* ap = a + (k >> 1);
        const int8_t* bp = b + (k >> 2);
        static const size_t uval = (1 << udeg);
        #pragma GCC unroll uval
        for (size_t i = 0; i < uval; i++) {
            LOAD_B(i*4)
            for (size_t j = 0; j < 8; j += 4) { // 'a' words, MSB first
                LOAD_A(i*8 + 4 - j)
                MAC_ITER_MIXED_8(4, int8_t, 2, int8_t)
            }
        }
    }

    size_t rem = (len - tile);
    if (rem > 0) {
        c += m_dotv_i4_i2_scalar_core(a + (tile >> 1), b + (tile >> 2), rem);
    }
    return c;
}

#undef MAC_ITER
#undef MAC_ITER_2
#undef MAC_ITER_4
#undef MAC_ITER_8
#undef MAC_ITER_16
#undef MAC_ITER_MIXED
#undef MAC_ITER_MIXED_2
#undef MAC_ITER_MIXED_4
#undef MAC_ITER_MIXED_8
#undef LOAD_SLICES
#undef LOAD_A
#undef LOAD_B
#undef LO_UNROLL

#endif

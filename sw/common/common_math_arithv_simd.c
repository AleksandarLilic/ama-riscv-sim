#include "common_math.h"
#include "common_math_dotv_scalar_core.h"
#include "common_math_simd_intrinsics.h"
#include "common_math_simd_v_load_store.h"

#ifdef __riscv_xsimd

INLINE_OPTION
void m_add_i16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len)
{
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
void m_add_i8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len)
{
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
void m_sub_i16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len)
{
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
        for (size_t i = len_s2; i < len; i++) c[i] = a[i] - b[i];
    }
}

INLINE_OPTION
void m_sub_i8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len)
{
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
        for (size_t i = len_s4; i < len; i++) c[i] = a[i] - b[i];
    }
}

INLINE_OPTION
void m_wmul_i16(
    const int16_t* a, const int16_t* b, int32_t* c, const size_t len)
{
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
void m_wmul_u16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len)
{
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
void m_wmul_i8(
    const int8_t* a, const int8_t* b, int16_t* c, const size_t len)
{
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
void m_wmul_u8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len)
{
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

#endif

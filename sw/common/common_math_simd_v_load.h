#ifndef COMMON_MATH_SIMD_V_LOAD_H
#define COMMON_MATH_SIMD_V_LOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#ifdef __riscv_xsimd

// -----------------------------------------------------------------------------
// SIMD vector loads
// -----------------------------------------------------------------------------

// 16-bit elements (2 lanes)
static INLINE
uint16x2_t v_load_uint16x2(const uint16_t* p) {
    uint16x2_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE
int16x2_t v_load_int16x2(const int16_t* p) {
    int16x2_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE
void v_store_int16x2(int16_t* p, int16x2_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

static INLINE
void v_store_uint16x2(uint16_t* p, uint16x2_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

// 8-bit elements (4 lanes)
static INLINE
uint8x4_t v_load_uint8x4(const uint8_t* p) {
    uint8x4_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE
int8x4_t v_load_int8x4(const int8_t* p) {
    int8x4_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE
void v_store_int8x4(int8_t* p, int8x4_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

static INLINE
void v_store_uint8x4(uint8_t* p, uint8x4_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

// 4-bit elements (8 lanes)
// note: input is (u)int8_t* because int4_t doesn't exist
// it's assumed that data is packed (2 elements per byte)
static INLINE
uint4x8_t v_load_uint4x8(const uint8_t* p) {
    uint4x8_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE
int4x8_t v_load_int4x8(const int8_t* p) {
    int4x8_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE
void v_store_int4x8(int8_t* p, int4x8_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

static INLINE
void v_store_uint4x8(uint8_t* p, uint4x8_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

// 2-bit elements (16 lanes)
// it's assumed that data is packed (4 elements per byte)
static INLINE
uint2x16_t v_load_uint2x16(const uint8_t* p) {
    uint2x16_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE
int2x16_t v_load_int2x16(const int8_t* p) {
    int2x16_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE
void v_store_int2x16(int8_t* p, int2x16_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

static INLINE
void v_store_uint2x16(uint8_t* p, uint2x16_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

#endif // __riscv_xsimd

#ifdef __cplusplus
}
#endif

#endif // COMMON_MATH_SIMD_H

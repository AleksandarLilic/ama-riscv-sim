#ifndef COMMON_MATH_SIMD_TYPES_H
#define COMMON_MATH_SIMD_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "common.h"

// simd data types
typedef struct { uint32_t v; } uint16x2_t;
typedef struct { uint32_t v; } int16x2_t;
typedef struct { uint32_t v; } uint8x4_t;
typedef struct { uint32_t v; } int8x4_t;
typedef struct { uint32_t v; } uint4x8_t;
typedef struct { uint32_t v; } int4x8_t;
typedef struct { uint32_t v; } uint2x16_t;
typedef struct { uint32_t v; } int2x16_t;

// simd wide data types
typedef union { uint64_t d; struct { uint32_t lo, hi; } w; } uint32x2_t;
typedef union { uint64_t d; struct { int32_t lo, hi; } w; } int32x2_t;
typedef union { uint64_t d; struct { uint16x2_t lo, hi; } w; } uint16x4_t;
typedef union { uint64_t d; struct { int16x2_t lo, hi; } w; } int16x4_t;
typedef union { uint64_t d; struct { uint8x4_t lo, hi; } w; } uint8x8_t;
typedef union { uint64_t d; struct { int8x4_t lo, hi; } w; } int8x8_t;
typedef union { uint64_t d; struct { uint4x8_t lo, hi; } w; } uint4x16_t;
typedef union { uint64_t d; struct { int4x8_t lo, hi; } w; } int4x16_t;
typedef union { uint64_t d; struct { uint2x16_t lo, hi; } w; } uint2x32_t;
typedef union { uint64_t d; struct { int2x16_t lo, hi; } w; } int2x32_t;

// reinterpret signedness within the same lane shape, preserving every bit
// convenience helpers, no instructions are emitted

// 32-bit vectors
static INLINE
uint16x2_t v_as_uint16x2(int16x2_t x) {
    uint16x2_t r;
    r.v = x.v;
    return r;
}

static INLINE
int16x2_t v_as_int16x2(uint16x2_t x) {
    int16x2_t r;
    r.v = x.v;
    return r;
}

static INLINE
uint8x4_t v_as_uint8x4(int8x4_t x) {
    uint8x4_t r;
    r.v = x.v;
    return r;
}

static INLINE
int8x4_t v_as_int8x4(uint8x4_t x) {
    int8x4_t r;
    r.v = x.v;
    return r;
}

static INLINE
uint4x8_t v_as_uint4x8(int4x8_t x) {
    uint4x8_t r;
    r.v = x.v;
    return r;
}

static INLINE
int4x8_t v_as_int4x8(uint4x8_t x) {
    int4x8_t r;
    r.v = x.v;
    return r;
}

static INLINE
uint2x16_t v_as_uint2x16(int2x16_t x) {
    uint2x16_t r;
    r.v = x.v;
    return r;
}

static INLINE
int2x16_t v_as_int2x16(uint2x16_t x) {
    int2x16_t r;
    r.v = x.v;
    return r;
}

// 64-bit register pairs
static INLINE
uint32x2_t v_as_uint32x2(int32x2_t x) {
    uint32x2_t r;
    r.d = x.d;
    return r;
}

static INLINE
int32x2_t v_as_int32x2(uint32x2_t x) {
    int32x2_t r;
    r.d = x.d;
    return r;
}

static INLINE
uint16x4_t v_as_uint16x4(int16x4_t x) {
    uint16x4_t r;
    r.d = x.d;
    return r;
}

static INLINE
int16x4_t v_as_int16x4(uint16x4_t x) {
    int16x4_t r;
    r.d = x.d;
    return r;
}

static INLINE
uint8x8_t v_as_uint8x8(int8x8_t x) {
    uint8x8_t r;
    r.d = x.d;
    return r;
}

static INLINE
int8x8_t v_as_int8x8(uint8x8_t x) {
    int8x8_t r;
    r.d = x.d;
    return r;
}

static INLINE
uint4x16_t v_as_uint4x16(int4x16_t x) {
    uint4x16_t r;
    r.d = x.d;
    return r;
}

static INLINE
int4x16_t v_as_int4x16(uint4x16_t x) {
    int4x16_t r;
    r.d = x.d;
    return r;
}

static INLINE
uint2x32_t v_as_uint2x32(int2x32_t x) {
    uint2x32_t r;
    r.d = x.d;
    return r;
}

static INLINE
int2x32_t v_as_int2x32(uint2x32_t x) {
    int2x32_t r;
    r.d = x.d;
    return r;
}

#ifdef __cplusplus
}
#endif

#endif // COMMON_MATH_SIMD_TYPES_H

#ifndef COMMON_MATH_H
#define COMMON_MATH_H

#include "common.h"

#define INLINE inline __attribute__((always_inline))

#ifdef FORCE_INLINE
#define INLINE_OPTION INLINE
#else
#define INLINE_OPTION
#endif

int32_t max(int32_t a, int32_t b);
uint32_t maxu(uint32_t a, uint32_t b);
int32_t min(int32_t a, int32_t b);
uint32_t minu(uint32_t a, uint32_t b);

#ifdef CUSTOM_ISA

// high level SIMD functions

// add & sub
void _simd_add_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len);
void _simd_add_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len);
void _simd_sub_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len);
void _simd_sub_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len);

// mul and mul unsigned
void _simd_mul_int16(
    const int16_t* a, const int16_t* b, int32_t* c, const size_t len);
void _simd_mul_int8(
    const int8_t* a, const int8_t* b, int16_t* c, const size_t len);
void _simd_mul_uint16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len);
void _simd_mul_uint8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len);

// dot product
int32_t _simd_dot_product_int16(
    const int16_t* a, const int16_t* b, const size_t len);
int32_t _simd_dot_product_int8(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t _simd_dot_product_int4(
    const int8_t* a, const int8_t* b, const size_t len);

// dot product w/ unpacking
int32_t _simd_dot_product_int16_int8(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t _simd_dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len);
//int32_t _simd_dot_product_int16_int2(
//    const int16_t* a, const int8_t* b, const size_t len);

int32_t _simd_dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t _simd_dot_product_int8_int2(
    const int8_t* a, const int8_t* b, const size_t len);

//int32_t _simd_dot_product_int4_int2(
//    const int8_t* a, const int8_t* b, const size_t len);

// low-level SIMD functions

// 16-bit elements (2 lanes)
static INLINE uint16x2_t v_load_uint16x2(const uint16_t* p) {
    uint16x2_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE int16x2_t v_load_int16x2(const int16_t* p) {
    int16x2_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE void v_store_int16x2(int16_t* p, int16x2_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

static INLINE void v_store_uint16x2(uint16_t* p, uint16x2_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

// 8-bit elements (4 lanes)
static INLINE uint8x4_t v_load_uint8x4(const uint8_t* p) {
    uint8x4_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE int8x4_t v_load_int8x4(const int8_t* p) {
    int8x4_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE void v_store_int8x4(int8_t* p, int8x4_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

static INLINE void v_store_uint8x4(uint8_t* p, uint8x4_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

// 4-bit elements (8 lanes)
// note: input is (u)int8_t* because int4_t doesn't exist
// it's assumed that data is packed (2 elements per byte)
static INLINE uint4x8_t v_load_uint4x8(const uint8_t* p) {
    uint4x8_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE int4x8_t v_load_int4x8(const int8_t* p) {
    int4x8_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE void v_store_int4x8(int8_t* p, int4x8_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

static INLINE void v_store_uint4x8(uint8_t* p, uint4x8_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

// 2-bit elements (16 lanes)
// it's assumed that data is packed (4 elements per byte)
static INLINE uint2x16_t v_load_uint2x16(const uint8_t* p) {
    uint2x16_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE int2x16_t v_load_int2x16(const int8_t* p) {
    int2x16_t r;
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(&r.v, p, 4);
    return r;
}

static INLINE void v_store_int2x16(int8_t* p, int2x16_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

static INLINE void v_store_uint2x16(uint8_t* p, uint2x16_t x) {
    p = __builtin_assume_aligned(p, 4);
    __builtin_memcpy(p, &x.v, 4);
}

// asm wrapper functions (aka intrinsics lite)
INLINE
int16x2_t _add16(const int16x2_t a, const int16x2_t b) {
    int16x2_t c;
    asm volatile("add16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int8x4_t _add8(const int8x4_t a, const int8x4_t b) {
    int8x4_t c;
    asm volatile("add8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int16x2_t _sub16(const int16x2_t a, const int16x2_t b) {
    int16x2_t c;
    asm volatile("sub16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int8x4_t _sub8(const int8x4_t a, const int8x4_t b) {
    int8x4_t c;
    asm volatile("sub8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int32x2_t _wmul16(const int16x2_t a, const int16x2_t b) {
    int32x2_t c;
    asm volatile("wmul16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
uint32x2_t _wmul16u(const uint16x2_t a, const uint16x2_t b) {
    uint32x2_t c;
    asm volatile("wmul16u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int16x4_t _wmul8(const int8x4_t a, const int8x4_t b) {
    int16x4_t c;
    asm volatile("wmul8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
uint16x4_t _wmul8u(const uint8x4_t a, const uint8x4_t b) {
    uint16x4_t c;
    asm volatile("wmul8u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
void _dot16(const int16x2_t a, const int16x2_t b, int32_t* c) {
    asm volatile("dot16 %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

INLINE
void _dot8(const int8x4_t a, const int8x4_t b, int32_t* c) {
    asm volatile("dot8 %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

INLINE
void _dot4(const int4x8_t a, const int4x8_t b, int32_t* c) {
    asm volatile("dot4 %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

#else // non-SIMD implementations

void add_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len);
void add_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len);
void sub_int16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len);
void sub_int8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len);

// mul and mul unsigned
void mul_int16(
    const int16_t* a, const int16_t* b, int32_t* c, const size_t len);
void mul_int8(
    const int8_t* a, const int8_t* b, int16_t* c, const size_t len);
void mul_uint16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len);
void mul_uint8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len);

int32_t dot_product_int16(const int16_t* a, const int16_t* b, const size_t len);
int32_t dot_product_int8(const int8_t* a, const int8_t* b, const size_t len);
int32_t dot_product_int4(const int8_t* a, const int8_t* b, const size_t len);

// dot product w/ unpacking
int32_t dot_product_int16_int8(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t dot_product_int8_int2(
    const int8_t* a, const int8_t* b, const size_t len);

#endif

#endif

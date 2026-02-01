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

#ifdef __riscv_xsimd

// -----------------------------------------------------------------------------
// high level SIMD functions
// -----------------------------------------------------------------------------

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

// dot product w/ widening
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

// these have the 'unrolled' optimized option, so '_core' is provided for
// 1. (#ifdef SIMD_UNROLL) last step if inputs are not multiple of tile size or
// 2. (#else) indirection in the regular version
int32_t _simd_dot_product_int8_core(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t _simd_dot_product_int8_int4_core(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t _simd_dot_product_int8_int2_core(
    const int8_t* a, const int8_t* b, const size_t len);

// -----------------------------------------------------------------------------
// low-level SIMD functions
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

// -----------------------------------------------------------------------------
// SIMD asm wrapper functions (aka intrinsics lite)
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// add & sub: wrap-around
static INLINE
int16x2_t _add16(const int16x2_t a, const int16x2_t b) {
    int16x2_t c;
    asm volatile("add16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int8x4_t _add8(const int8x4_t a, const int8x4_t b) {
    int8x4_t c;
    asm volatile("add8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int16x2_t _sub16(const int16x2_t a, const int16x2_t b) {
    int16x2_t c;
    asm volatile("sub16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int8x4_t _sub8(const int8x4_t a, const int8x4_t b) {
    int8x4_t c;
    asm volatile("sub8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

// -----------------------------------------------------------------------------
// add & sub: saturating
static INLINE
int16x2_t _qadd16(const int16x2_t a, const int16x2_t b) {
    int16x2_t c;
    asm volatile("qadd16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint16x2_t _qadd16u(const uint16x2_t a, const uint16x2_t b) {
    uint16x2_t c;
    asm volatile("qadd16u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int8x4_t _qadd8(const int8x4_t a, const int8x4_t b) {
    int8x4_t c;
    asm volatile("qadd8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint8x4_t _qadd8u(const uint8x4_t a, const uint8x4_t b) {
    uint8x4_t c;
    asm volatile("qadd8u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int16x2_t _qsub16(const int16x2_t a, const int16x2_t b) {
    int16x2_t c;
    asm volatile("qsub16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint16x2_t _qsub16u(const uint16x2_t a, const uint16x2_t b) {
    uint16x2_t c;
    asm volatile("qsub16u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int8x4_t _qsub8(const int8x4_t a, const int8x4_t b) {
    int8x4_t c;
    asm volatile("qsub8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint8x4_t _qsub8u(const uint8x4_t a, const uint8x4_t b) {
    uint8x4_t c;
    asm volatile("qsub8u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

// -----------------------------------------------------------------------------
// mul and mul unsigned
static INLINE
int32x2_t _wmul16(const int16x2_t a, const int16x2_t b) {
    int32x2_t c;
    asm volatile("wmul16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint32x2_t _wmul16u(const uint16x2_t a, const uint16x2_t b) {
    uint32x2_t c;
    asm volatile("wmul16u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int16x4_t _wmul8(const int8x4_t a, const int8x4_t b) {
    int16x4_t c;
    asm volatile("wmul8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint16x4_t _wmul8u(const uint8x4_t a, const uint8x4_t b) {
    uint16x4_t c;
    asm volatile("wmul8u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

// -----------------------------------------------------------------------------
// dot product
static INLINE
void _dot16(const int16x2_t a, const int16x2_t b, int32_t* c) {
    asm volatile("dot16 %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

static INLINE
void _dot16u(const uint16x2_t a, const uint16x2_t b, uint32_t* c) {
    asm volatile("dot16u %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

static INLINE
void _dot8(const int8x4_t a, const int8x4_t b, int32_t* c) {
    asm volatile("dot8 %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

static INLINE
void _dot8u(const uint8x4_t a, const uint8x4_t b, int32_t* c) {
    asm volatile("dot8u %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

static INLINE
void _dot4(const int4x8_t a, const int4x8_t b, int32_t* c) {
    asm volatile("dot4 %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

static INLINE
void _dot4u(const uint4x8_t a, const uint4x8_t b, int32_t* c) {
    asm volatile("dot4u %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

static INLINE
void _dot2(const int2x16_t a, const int2x16_t b, int32_t* c) {
    asm volatile("dot2 %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

static INLINE
void _dot2u(const uint2x16_t a, const uint2x16_t b, int32_t* c) {
    asm volatile("dot2u %0, %1, %2" : "+r"(*c) : "r"(a), "r"(b));
}

// -----------------------------------------------------------------------------
// compare
static INLINE
int16x2_t _min16(const int16x2_t a, const int16x2_t b) {
    int16x2_t c;
    asm volatile("min16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint16x2_t _min16u(const uint16x2_t a, const uint16x2_t b) {
    uint16x2_t c;
    asm volatile("min16u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int8x4_t _min8(const int8x4_t a, const int8x4_t b) {
    int8x4_t c;
    asm volatile("min8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint8x4_t _min8u(const uint8x4_t a, const uint8x4_t b) {
    uint8x4_t c;
    asm volatile("min8u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int16x2_t _max16(const int16x2_t a, const int16x2_t b) {
    int16x2_t c;
    asm volatile("max16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint16x2_t _max16u(const uint16x2_t a, const uint16x2_t b) {
    uint16x2_t c;
    asm volatile("max16u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int8x4_t _max8(const int8x4_t a, const int8x4_t b) {
    int8x4_t c;
    asm volatile("max8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint8x4_t _max8u(const uint8x4_t a, const uint8x4_t b) {
    uint8x4_t c;
    asm volatile("max8u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

// -----------------------------------------------------------------------------
// shift
static INLINE
int16x2_t _slli16(const int16x2_t a, const int32_t imm) {
    int16x2_t c;
    asm volatile("slli16 %0, %1, %2" : "=r"(c) : "r"(a), "i"(imm));
    return c;
}

static INLINE
int8x4_t _slli8(const int8x4_t a, const int32_t imm) {
    int8x4_t c;
    asm volatile("slli8 %0, %1, %2" : "=r"(c) : "r"(a), "i"(imm));
    return c;
}

static INLINE
int16x2_t _srli16(const int16x2_t a, const int32_t imm) {
    int16x2_t c;
    asm volatile("srli16 %0, %1, %2" : "=r"(c) : "r"(a), "i"(imm));
    return c;
}

static INLINE
int8x4_t _srli8(const int8x4_t a, const int32_t imm) {
    int8x4_t c;
    asm volatile("srli8 %0, %1, %2" : "=r"(c) : "r"(a), "i"(imm));
    return c;
}

static INLINE
int16x2_t _srai16(const int16x2_t a, const int32_t imm) {
    int16x2_t c;
    asm volatile("srai16 %0, %1, %2" : "=r"(c) : "r"(a), "i"(imm));
    return c;
}

static INLINE
int8x4_t _srai8(const int8x4_t a, const int32_t imm) {
    int8x4_t c;
    asm volatile("srai8 %0, %1, %2" : "=r"(c) : "r"(a), "i"(imm));
    return c;
}

// -----------------------------------------------------------------------------
// data formatting - widen
static INLINE
int32x2_t _widen16(const int16x2_t a) {
    int32x2_t c;
    asm volatile("widen16 %0, %1" : "=r"(c.d) : "r"(a));
    return c;
}

static INLINE
uint32x2_t _widen16u(const uint16x2_t a) {
    uint32x2_t c;
    asm volatile("widen16u %0, %1" : "=r"(c.d) : "r"(a));
    return c;
}

static INLINE
int16x4_t _widen8(const int8x4_t a) {
    int16x4_t c;
    asm volatile("widen8 %0, %1" : "=r"(c.d) : "r"(a));
    return c;
}

static INLINE
uint16x4_t _widen8u(const uint8x4_t a) {
    uint16x4_t c;
    asm volatile("widen8u %0, %1" : "=r"(c.d) : "r"(a));
    return c;
}

static INLINE
int8x8_t _widen4(const int4x8_t a) {
    int8x8_t c;
    asm volatile("widen4 %0, %1" : "=r"(c.d) : "r"(a));
    return c;
}

static INLINE
uint8x8_t _widen4u(const uint4x8_t a) {
    uint8x8_t c;
    asm volatile("widen4u %0, %1" : "=r"(c.d) : "r"(a));
    return c;
}

static INLINE
int4x16_t _widen2(const int2x16_t a) {
    int4x16_t c;
    asm volatile("widen2 %0, %1" : "=r"(c.d) : "r"(a));
    return c;
}

static INLINE
uint4x16_t _widen2u(const uint2x16_t a) {
    uint4x16_t c;
    asm volatile("widen2u %0, %1" : "=r"(c.d) : "r"(a));
    return c;
}

// -----------------------------------------------------------------------------
// data formatting - narrow truncating
static INLINE
uint16x2_t _narrow32(const uint32_t a, const uint32_t b) {
    uint16x2_t c;
    asm volatile("narrow32 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint8x4_t _narrow16(const uint16x2_t a, const uint16x2_t b) {
    uint8x4_t c;
    asm volatile("narrow16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint4x8_t _narrow8(const uint8x4_t a, const uint8x4_t b) {
    uint4x8_t c;
    asm volatile("narrow8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint2x16_t _narrow4(const uint4x8_t a, const uint4x8_t b) {
    uint2x16_t c;
    asm volatile("narrow4 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

// -----------------------------------------------------------------------------
// data formatting - narrow saturating
static INLINE
int16x2_t _qnarrow32(const int32_t a, const int32_t b) {
    int16x2_t c;
    asm volatile("qnarrow32 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint16x2_t _qnarrow32u(const uint32_t a, const uint32_t b) {
    uint16x2_t c;
    asm volatile("qnarrow32u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int8x4_t _qnarrow16(const int16x2_t a, const int16x2_t b) {
    int8x4_t c;
    asm volatile("qnarrow16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint8x4_t _qnarrow16u(const uint16x2_t a, const uint16x2_t b) {
    uint8x4_t c;
    asm volatile("qnarrow16u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int4x8_t _qnarrow8(const int8x4_t a, const int8x4_t b) {
    int4x8_t c;
    asm volatile("qnarrow8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint4x8_t _qnarrow8u(const uint8x4_t a, const uint8x4_t b) {
    uint4x8_t c;
    asm volatile("qnarrow8u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
int2x16_t _qnarrow4(const int4x8_t a, const int4x8_t b) {
    int2x16_t c;
    asm volatile("qnarrow4 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

static INLINE
uint2x16_t _qnarrow4u(const uint4x8_t a, const uint4x8_t b) {
    uint2x16_t c;
    asm volatile("qnarrow4u %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

// -----------------------------------------------------------------------------
// data formatting - swap anti-diagonal
static INLINE
uint16x4_t _swapad16(uint16x2_t a, uint16x2_t b) {
    uint16x4_t c;
    asm volatile("swapad16 %0, %1, %2" : "=r"(c): "r"(a), "r"(b));
    return c;
}

static INLINE
uint8x8_t _swapad8(uint8x4_t a, uint8x4_t b) {
    uint8x8_t c;
    asm volatile("swapad8 %0, %1, %2" : "=r"(c): "r"(a), "r"(b));
    return c;
}

static INLINE
uint4x16_t _swapad4(uint4x8_t a, uint4x8_t b) {
    uint4x16_t c;
    asm volatile("swapad4 %0, %1, %2" : "=r"(c): "r"(a), "r"(b));
    return c;
}

static INLINE
uint2x32_t _swapad2(uint2x16_t a, uint2x16_t b) {
    uint2x32_t c;
    asm volatile("swapad2 %0, %1, %2" : "=r"(c): "r"(a), "r"(b));
    return c;
}

// -----------------------------------------------------------------------------
// scalar-vector dup (broadcast scalar into all lanes)
static INLINE
uint16x2_t _dup16(uint16_t scalar) {
    uint16x2_t c;
    asm volatile("dup16 %0, %1" : "=r"(c) : "r"((uint32_t)scalar));
    return c;
}

static INLINE
uint8x4_t _dup8(uint8_t scalar) {
    uint8x4_t c;
    asm volatile("dup8 %0, %1" : "=r"(c) : "r"((uint32_t)scalar));
    return c;
}

static INLINE
uint4x8_t _dup4(uint8_t scalar) {
    uint4x8_t c;
    asm volatile("dup4 %0, %1" : "=r"(c) : "r"((uint32_t)(scalar & 0xFu)));
    return c;
}

static INLINE
uint2x16_t _dup2(uint8_t scalar) {
    uint2x16_t c;
    asm volatile("dup2 %0, %1" : "=r"(c) : "r"((uint32_t)(scalar & 0x3u)));
    return c;
}

// -----------------------------------------------------------------------------
// scalar-vector vins (insert scalar into one lane of vector, RMW)
static INLINE
uint16x2_t _vins16(uint16x2_t vec, uint16_t scalar, size_t lane_idx) {
    uint16x2_t r = vec;
    asm volatile("vins16 %0, %1, %2" : "+r"(r) :
                 "r"((uint32_t)scalar), "i"(lane_idx));
    return r;
}

static INLINE
uint8x4_t _vins8(uint8x4_t vec, uint8_t scalar, size_t lane_idx) {
    uint8x4_t r = vec;
    asm volatile("vins8 %0, %1, %2" : "+r"(r) :
                 "r"((uint32_t)scalar), "i"(lane_idx));
    return r;
}

static INLINE
uint4x8_t _vins4(uint4x8_t vec, uint8_t scalar, size_t lane_idx) {
    uint4x8_t r = vec;
    asm volatile("vins4 %0, %1, %2" : "+r"(r) :
                 "r"((uint32_t)(scalar & 0xFu)), "i"(lane_idx));
    return r;
}

static INLINE
uint2x16_t _vins2(uint2x16_t vec, uint8_t scalar, size_t lane_idx) {
    uint2x16_t r = vec;
    asm volatile("vins2 %0, %1, %2" : "+r"(r) :
                 "r"((uint32_t)(scalar & 0x3u)), "i"(lane_idx));
    return r;
}

#else // non __riscv_xsimd implementations

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

#endif // __riscv_xsimd

#endif // COMMON_MATH_H

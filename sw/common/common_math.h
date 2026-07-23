#ifndef COMMON_MATH_H
#define COMMON_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#ifdef PARTIAL_ZBB_SUPPORT
static INLINE int32_t max(int32_t a, int32_t b) {
    int32_t c;
    asm volatile(
        ".insn r 0x33, 0x6, 0x5, %0, %1, %2"
        : "=r"(c)
        : "r"(a), "r"(b)
    );
    return c;
}

static INLINE uint32_t maxu(uint32_t a, uint32_t b) {
    uint32_t c;
    asm volatile(
        ".insn r 0x33, 0x7, 0x5, %0, %1, %2"
        : "=r"(c)
        : "r"(a), "r"(b)
    );
    return c;
}

static INLINE int32_t min(int32_t a, int32_t b) {
    int32_t c;
    asm volatile(
        ".insn r 0x33, 0x4, 0x5, %0, %1, %2"
        : "=r"(c)
        : "r"(a), "r"(b)
    );
    return c;
}

static INLINE uint32_t minu(uint32_t a, uint32_t b) {
    uint32_t c;
    asm volatile(
        ".insn r 0x33, 0x5, 0x5, %0, %1, %2"
        : "=r"(c)
        : "r"(a), "r"(b)
    );
    return c;
}

#else
static INLINE int32_t max(int32_t a, int32_t b) {
    return a > b ? a : b;
}

static INLINE uint32_t maxu(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

static INLINE int32_t min(int32_t a, int32_t b) {
    return a < b ? a : b;
}

static INLINE uint32_t minu(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}
#endif

#ifdef __riscv_xsimd
// simd provides its own functions
#include "common_math_simd.h"

#else
// scalar functions
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
int32_t dot_product_int2(const int8_t* a, const int8_t* b, const size_t len);

// dot product w/ unpacking
int32_t dot_product_int16_int8(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t dot_product_int16_int2(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t dot_product_int8_int2(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t dot_product_int4_int2(
    const int8_t* a, const int8_t* b, const size_t len);

#endif // __riscv_xsimd

#ifdef __cplusplus
}
#endif

#endif // COMMON_MATH_H

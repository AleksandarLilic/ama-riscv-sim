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

// -----------------------------------------------------------------------------
// public API
// -----------------------------------------------------------------------------

// add & sub
void m_add_i16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len);
void m_add_i8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len);
void m_sub_i16(
    const int16_t* a, const int16_t* b, int16_t* c, const size_t len);
void m_sub_i8(
    const int8_t* a, const int8_t* b, int8_t* c, const size_t len);

// mul and mul unsigned
void m_mul_i16(
    const int16_t* a, const int16_t* b, int32_t* c, const size_t len);
void m_mul_i8(
    const int8_t* a, const int8_t* b, int16_t* c, const size_t len);
void m_mul_u16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len);
void m_mul_u8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len);

// dot product
int32_t m_dotv_i16_i16(
    const int16_t* a, const int16_t* b, const size_t len);
int32_t m_dotv_i8_i8(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i4_i4(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i2_i2(
    const int8_t* a, const int8_t* b, const size_t len);

// dot product w/ unpacking
int32_t m_dotv_i16_i8(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i16_i4(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i16_i2(
    const int16_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i8_i4(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i8_i2(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t m_dotv_i4_i2(
    const int8_t* a, const int8_t* b, const size_t len);

#ifdef __riscv_xsimd
// SIMD intrinsics, and the routines that have no scalar implementation yet
#include "common_math_simd.h"
#endif

#ifdef __cplusplus
}
#endif

#endif // COMMON_MATH_H

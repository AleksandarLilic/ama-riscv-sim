#ifndef COMMON_MATH_H
#define COMMON_MATH_H

#include "common.h"

#define INLINE inline __attribute__((always_inline))
#ifdef FORCE_INLINE
#define INLINE_OPTION INLINE
#else
#define INLINE_OPTION
#endif

#define PARTIAL_ZBB_SUPPORT
int32_t max(int32_t a, int32_t b);
uint32_t maxu(uint32_t a, uint32_t b);
int32_t min(int32_t a, int32_t b);
uint32_t minu(uint32_t a, uint32_t b);

#ifdef CUSTOM_ISA

// paired registers for unpacking ISA, RDP has to be +1 of RD
#define RD_1 t5 // x30
#define RDP_1 t6 // x31, the paired register
#define RD_2 t3 // x28
#define RDP_2 t4 // x29, the paired register
#define RD_3 t1 // x6
#define RDP_3 t2 // x7, the paired register

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

// asm wrapper functions
INLINE
int32_t add16(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("add16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int32_t add8(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("add8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int32_t sub16(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("sub16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int32_t sub8(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("sub8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int32_t dot16(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("dot16 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int32_t dot8(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("dot8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

INLINE
int32_t dot4(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("dot4 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
}

#else

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

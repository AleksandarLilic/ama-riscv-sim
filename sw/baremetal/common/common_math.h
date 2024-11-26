#include "common.h"

#define INLINE inline __attribute__((always_inline))
#ifdef FORCE_INLINE
#define INLINE_OPTION INLINE
#else
#define INLINE_OPTION
#endif

#ifdef CUSTOM_ISA

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
//int32_t _simd_dot_product_int8_int2(
//    const int8_t* a, const int8_t* b, const size_t len);

//int32_t _simd_dot_product_int4_int2(
//    const int8_t* a, const int8_t* b, const size_t len);

// add & sub
//int32_t _simd_add_int16(const int16_t* a, const int16_t* b, const size_t len);
//int32_t _simd_add_int8(const int8_t* a, const int8_t* b, const size_t len);
//int32_t _simd_sub_int16(const int16_t* a, const int16_t* b, const size_t len);
//int32_t _simd_sub_int8(const int8_t* a, const int8_t* b, const size_t len);

// mul and mul unsigned
//int32_t _simd_mul_int16(const int16_t* a, const int16_t* b, const size_t len);
//int32_t _simd_mul_int8(const int8_t* a, const int8_t* b, const size_t len);
//int32_t _simd_mulu_int16(const uint16_t* a, const uint16_t* b, const size_t len);
//int32_t _simd_mulu_int8(const uint8_t* a, const uint8_t* b, const size_t len);

// asm wrapper functions
INLINE int32_t fma16(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("fma16 %0, %1, %2"
                 : "=r"(c)
                 : "r"(a), "r"(b));
    return c;
}

INLINE int32_t fma8(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("fma8 %0, %1, %2"
                 : "=r"(c)
                 : "r"(a), "r"(b));
    return c;
}

INLINE int32_t fma4(const int32_t a, const int32_t b) {
    int32_t c;
    asm volatile("fma4 %0, %1, %2"
                 : "=r"(c)
                 : "r"(a), "r"(b));
    return c;
}

#else

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

#endif

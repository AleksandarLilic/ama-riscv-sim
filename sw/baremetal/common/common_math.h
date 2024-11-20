#include "common.h"

#ifdef CUSTOM_ISA
int32_t _simd_dot_product_int16(
    const int16_t* a, const int16_t* b, const size_t len);
int32_t _simd_dot_product_int8(
    const int8_t* a, const int8_t* b, const size_t len);
int32_t _simd_dot_product_int4(
    const int8_t* a, const int8_t* b, const size_t len);

// asm wrappers functions
int32_t fma16(const int32_t a, const int32_t b);
int32_t fma8(const int32_t a, const int32_t b);
int32_t fma4(const int32_t a, const int32_t b);

#else
int32_t dot_product_int16(const int16_t* a, const int16_t* b, const size_t len);
int32_t dot_product_int8(const int8_t* a, const int8_t* b, const size_t len);
int32_t dot_product_int4(const int8_t* a, const int8_t* b, const size_t len);
#endif

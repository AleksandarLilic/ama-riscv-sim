#include "common_math.h"
#include "common_math_scalar_core.h"

#if !defined(__riscv_xsimd) && !defined(LOAD_OPT)

INLINE_OPTION
void add_int16(const int16_t* a, const int16_t* b, int16_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] + b[k];
}

INLINE_OPTION
void add_int8(const int8_t* a, const int8_t* b, int8_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] + b[k];
}

INLINE_OPTION
void sub_int16(const int16_t* a, const int16_t* b, int16_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] - b[k];
}

INLINE_OPTION
void sub_int8(const int8_t* a, const int8_t* b, int8_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] - b[k];
}

INLINE_OPTION
void mul_int16(const int16_t* a, const int16_t* b, int32_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
}

INLINE_OPTION
void mul_int8(const int8_t* a, const int8_t* b, int16_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
}

INLINE_OPTION
void mul_uint16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
}

INLINE_OPTION
void mul_uint8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] * b[k];
}

INLINE_OPTION
int32_t dot_product_int16(const int16_t* a, const int16_t* b, const size_t len)
{
    return dot_product_int16_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t dot_product_int8(const int8_t* a, const int8_t* b, const size_t len)
{
    return dot_product_int8_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t dot_product_int4(const int8_t* a, const int8_t* b, const size_t len)
{
    return dot_product_int4_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t dot_product_int2(const int8_t* a, const int8_t* b, const size_t len)
{
    return dot_product_int2_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t dot_product_int16_int8(
    const int16_t* a, const int8_t* b, const size_t len)
{
    return dot_product_int16_int8_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t dot_product_int16_int4(
    const int16_t* a, const int8_t* b, const size_t len)
{
    return dot_product_int16_int4_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t dot_product_int16_int2(
    const int16_t* a, const int8_t* b, const size_t len)
{
    return dot_product_int16_int2_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t dot_product_int8_int4(
    const int8_t* a, const int8_t* b, const size_t len)
{
    return dot_product_int8_int4_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t dot_product_int8_int2(
    const int8_t* a, const int8_t* b, const size_t len)
{
    return dot_product_int8_int2_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t dot_product_int4_int2(
    const int8_t* a, const int8_t* b, const size_t len)
{
    return dot_product_int4_int2_scalar_core(a, b, len);
}

#endif

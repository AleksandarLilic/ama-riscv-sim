#ifndef COMMON_MATH_SCALAR_CORE_H
#define COMMON_MATH_SCALAR_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

// -----------------------------------------------------------------------------
// scalar dot product cores
// -----------------------------------------------------------------------------
static INLINE
int32_t dot_product_int16_scalar_core(
    const int16_t* a, const int16_t* b, const size_t len)
{
    int32_t c = 0;
    for (size_t k = 0; k < len; k++) c += a[k] * b[k];
    return c;
}

static INLINE
int32_t dot_product_int8_scalar_core(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    for (size_t k = 0; k < len; k++) c += a[k] * b[k];
    return c;
}

static INLINE
int32_t dot_product_int4_scalar_core(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    int8_t al, bl;
    size_t len_bytes = len >> 1; // len passed in as number of nibbles
    for (size_t k = 0; k < len_bytes; k++) {
        al = a[k];
        bl = b[k];
        c += (int8_t)(al >> 4) * (int8_t)(bl >> 4);
        al <<= 4;
        bl <<= 4;
        c += (int8_t)(al >> 4) * (int8_t)(bl >> 4);
    }
    return c;
}

static INLINE
int32_t dot_product_int2_scalar_core(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    int8_t al, bl;
    size_t len_bytes = len >> 2; // len passed in as number of crumbs
    for (size_t k = 0; k < len_bytes; k++) {
        al = a[k];
        bl = b[k];
        c += (int8_t)(al >> 6) * (int8_t)(bl >> 6);
        al <<= 2;
        bl <<= 2;
        c += (int8_t)(al >> 6) * (int8_t)(bl >> 6);
        al <<= 2;
        bl <<= 2;
        c += (int8_t)(al >> 6) * (int8_t)(bl >> 6);
        al <<= 2;
        bl <<= 2;
        c += (int8_t)(al >> 6) * (int8_t)(bl >> 6);
    }
    return c;
}

static INLINE
int32_t dot_product_int16_int8_scalar_core(
    const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    for (size_t k = 0; k < len; k++) c += a[k] * (int16_t)b[k];
    return c;
}

static INLINE
int32_t dot_product_int16_int4_scalar_core(
    const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    int8_t bl;
    for (size_t k = 0; k < len; k += 2) {
        bl = b[k>>1];
        c += a[k+1] * (int16_t)(bl >> 4);
        bl <<= 4;
        c += a[k] * (int16_t)(bl >> 4);
    }
    return c;
}

static INLINE
int32_t dot_product_int16_int2_scalar_core(
    const int16_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    int8_t bl;
    for (size_t k = 0; k < len; k += 4) {
        bl = b[k>>2];
        c += a[k+3] * (int16_t)(bl >> 6);
        bl <<= 2;
        c += a[k+2] * (int16_t)(bl >> 6);
        bl <<= 2;
        c += a[k+1] * (int16_t)(bl >> 6);
        bl <<= 2;
        c += a[k] * (int16_t)(bl >> 6);
    }
    return c;
}

static INLINE
int32_t dot_product_int8_int4_scalar_core(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    int8_t bl;
    for (size_t k = 0; k < len; k += 2) {
        bl = b[k>>1];
        c += a[k+1] * (int8_t)(bl >> 4);
        bl <<= 4;
        c += a[k] * (int8_t)(bl >> 4);
    }
    return c;
}

static INLINE
int32_t dot_product_int8_int2_scalar_core(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    int8_t bl;
    for (size_t k = 0; k < len; k += 4) {
        bl = b[k>>2];
        c += a[k+3] * (int8_t)(bl >> 6);
        bl <<= 2;
        c += a[k+2] * (int8_t)(bl >> 6);
        bl <<= 2;
        c += a[k+1] * (int8_t)(bl >> 6);
        bl <<= 2;
        c += a[k] * (int8_t)(bl >> 6);
    }
    return c;
}

static INLINE
int32_t dot_product_int4_int2_scalar_core(
    const int8_t* a, const int8_t* b, const size_t len)
{
    int32_t c = 0;
    int8_t al, bl;
    for (size_t k = 0; k < len; k += 4) {
        al = a[(k>>1) + 1];
        bl = b[k>>2];
        c += (int8_t)(al >> 4) * (int8_t)(bl >> 6);
        al <<= 4;
        bl <<= 2;
        c += (int8_t)(al >> 4) * (int8_t)(bl >> 6);
        al = a[k>>1];
        bl <<= 2;
        c += (int8_t)(al >> 4) * (int8_t)(bl >> 6);
        al <<= 4;
        bl <<= 2;
        c += (int8_t)(al >> 4) * (int8_t)(bl >> 6);
    }
    return c;
}

#ifdef __cplusplus
}
#endif

#endif // COMMON_MATH_SCALAR_CORE_H

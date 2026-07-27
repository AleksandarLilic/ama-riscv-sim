#include "common_math.h"
#include "common_math_scalar_core.h"

#if !defined(__riscv_xsimd) && !defined(LOAD_OPT)

INLINE_OPTION
void m_add_i16(const int16_t* a, const int16_t* b, int16_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] + b[k];
}

INLINE_OPTION
void m_add_i8(const int8_t* a, const int8_t* b, int8_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] + b[k];
}

INLINE_OPTION
void m_sub_i16(const int16_t* a, const int16_t* b, int16_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] - b[k];
}

INLINE_OPTION
void m_sub_i8(const int8_t* a, const int8_t* b, int8_t* c, const size_t len)
{
    for (size_t k = 0; k < len; k++) c[k] = a[k] - b[k];
}

INLINE_OPTION
void m_mul_i16(const int16_t* a, const int16_t* b, int32_t* c, const size_t len)
{
    m_mul_i16_scalar_core(a, b, c, len);
}

INLINE_OPTION
void m_mul_i8(const int8_t* a, const int8_t* b, int16_t* c, const size_t len)
{
    m_mul_i8_scalar_core(a, b, c, len);
}

INLINE_OPTION
void m_mul_u16(
    const uint16_t* a, const uint16_t* b, uint32_t* c, const size_t len)
{
    m_mul_u16_scalar_core(a, b, c, len);
}

INLINE_OPTION
void m_mul_u8(
    const uint8_t* a, const uint8_t* b, uint16_t* c, const size_t len)
{
    m_mul_u8_scalar_core(a, b, c, len);
}

INLINE_OPTION
int32_t m_dotv_i16_i16(const int16_t* a, const int16_t* b, const size_t len)
{
    return m_dotv_i16_i16_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i8_i8(const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i8_i8_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i4_i4(const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i4_i4_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i2_i2(const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i2_i2_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i16_i8(
    const int16_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i16_i8_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i16_i4(
    const int16_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i16_i4_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i16_i2(
    const int16_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i16_i2_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i8_i4(
    const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i8_i4_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i8_i2(
    const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i8_i2_scalar_core(a, b, len);
}

INLINE_OPTION
int32_t m_dotv_i4_i2(
    const int8_t* a, const int8_t* b, const size_t len)
{
    return m_dotv_i4_i2_scalar_core(a, b, len);
}

#endif

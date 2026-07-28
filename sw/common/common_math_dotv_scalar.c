#include "common_math.h"
#include "common_math_dotv_scalar_core.h"

#if !defined(__riscv_xsimd) && !defined(LOAD_OPT)

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

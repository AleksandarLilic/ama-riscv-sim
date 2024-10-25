#include "common.h"

int32_t dot_product_int16(int16_t* a, int16_t* b, size_t len);
int32_t dot_product_int8(int8_t* a, int8_t* b, size_t len);
int32_t dot_product_int4(int8_t* a, int8_t* b, size_t len);

#ifdef CUSTOM_ISA
int32_t fma16(const int32_t a, const int32_t b);
int32_t fma8(const int32_t a, const int32_t b);
int32_t fma4(const int32_t a, const int32_t b);
#endif

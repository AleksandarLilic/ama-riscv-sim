#include "defines.h"
#include "core.h"

template <int vbits, bool vsigned>
uint32_t core::alu_c_dot_template(uint32_t a, uint32_t b, uint32_t c) {
    constexpr int e = (32 / vbits);
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ");
    #endif

    for (int i = 0; i < e; i++) {
        int32_t val_a = extract_val<vbits, vsigned>(a);
        int32_t val_b = extract_val<vbits, vsigned>(b);

        #ifdef DASM_EN
        simd_ss_append(val_a, val_b);
        #endif

        res += (val_a * val_b);
        // shift down for next iteration
        a >>= vbits;
        b >>= vbits;
    }
    res += c;

    #ifdef DASM_EN
    simd_ss_finish("]", "]", res, c);
    #endif

    return TO_U32(res);
}

uint32_t core::alu_c_dot16(uint32_t a, uint32_t b, uint32_t c)  {
    return alu_c_dot_template<16, true>(a, b, c);
}

uint32_t core::alu_c_dot16u(uint32_t a, uint32_t b, uint32_t c) {
    return alu_c_dot_template<16, false>(a, b, c);
}

uint32_t core::alu_c_dot8(uint32_t a, uint32_t b, uint32_t c)   {
    return alu_c_dot_template<8, true>(a, b, c);
}

uint32_t core::alu_c_dot8u(uint32_t a, uint32_t b, uint32_t c)  {
    return alu_c_dot_template<8, false>(a, b, c);
}

uint32_t core::alu_c_dot4(uint32_t a, uint32_t b, uint32_t c)   {
    return alu_c_dot_template<4, true>(a, b, c);
}

uint32_t core::alu_c_dot4u(uint32_t a, uint32_t b, uint32_t c)  {
    return alu_c_dot_template<4, false>(a, b, c);
}

uint32_t core::alu_c_dot2(uint32_t a, uint32_t b, uint32_t c)   {
    return alu_c_dot_template<2, true>(a, b, c);
}

uint32_t core::alu_c_dot2u(uint32_t a, uint32_t b, uint32_t c)  {
    return alu_c_dot_template<2, false>(a, b, c);
}

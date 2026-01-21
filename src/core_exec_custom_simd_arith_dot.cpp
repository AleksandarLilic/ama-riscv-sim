#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

template <size_t vbits, bool vsigned>
uint32_t core::alu_c_dot_op(uint32_t a, uint32_t b, uint32_t c) {
    constexpr size_t e = (32 / vbits);
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
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
    return alu_c_dot_op<16, true>(a, b, c);
}

uint32_t core::alu_c_dot16u(uint32_t a, uint32_t b, uint32_t c) {
    return alu_c_dot_op<16, false>(a, b, c);
}

uint32_t core::alu_c_dot8(uint32_t a, uint32_t b, uint32_t c)   {
    return alu_c_dot_op<8, true>(a, b, c);
}

uint32_t core::alu_c_dot8u(uint32_t a, uint32_t b, uint32_t c)  {
    return alu_c_dot_op<8, false>(a, b, c);
}

uint32_t core::alu_c_dot4(uint32_t a, uint32_t b, uint32_t c)   {
    return alu_c_dot_op<4, true>(a, b, c);
}

uint32_t core::alu_c_dot4u(uint32_t a, uint32_t b, uint32_t c)  {
    return alu_c_dot_op<4, false>(a, b, c);
}

uint32_t core::alu_c_dot2(uint32_t a, uint32_t b, uint32_t c)   {
    return alu_c_dot_op<2, true>(a, b, c);
}

uint32_t core::alu_c_dot2u(uint32_t a, uint32_t b, uint32_t c)  {
    return alu_c_dot_op<2, false>(a, b, c);
}

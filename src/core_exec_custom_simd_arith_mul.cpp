#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

// each lane: compute 2*vbits product, then keep lower (upper=false) or
// upper (upper=true) vbits and pack back into the 32-bit output register.
template <size_t vbits, bool vsigned, bool upper>
uint32_t core::alu_c_mul_op(uint32_t a, uint32_t b) {
    constexpr size_t e = lane<vbits>::count;
    constexpr uint32_t lane_mask = lane<vbits>::mask;

    uint32_t result = 0;

    #ifdef DASM_EN
    simd_ss_init_cab();
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t val_a = extract_val<vbits, vsigned>(a);
        int32_t val_b = extract_val<vbits, vsigned>(b);

        int32_t product = (val_a * val_b);

        // extract lower or upper vbits of the 2*vbits product
        uint32_t lane_result;
        if constexpr (upper) {
            lane_result = (TO_U32(product) >> vbits) & lane_mask;
        } else {
            lane_result = TO_U32(product) & lane_mask;
        }

        result |= (lane_result << (i * vbits));

        #ifdef DASM_EN
        simd_ss_append_ab(val_a, val_b);
        dasm.simd_c << TO_I32(lane_result) << " ";
        #endif

        a >>= vbits;
        b >>= vbits;
    }

    #ifdef DASM_EN
    simd_ss_finish_cab();
    #endif

    return result;
}

// instantiations
uint32_t core::alu_c_mul16(uint32_t a, uint32_t b) {
    return alu_c_mul_op<16, true,  false>(a, b);
}
uint32_t core::alu_c_mul8(uint32_t a, uint32_t b) {
    return alu_c_mul_op<8,  true,  false>(a, b);
}
uint32_t core::alu_c_mulh16(uint32_t a, uint32_t b) {
    return alu_c_mul_op<16, true,  true >(a, b);
}
uint32_t core::alu_c_mulh16u(uint32_t a, uint32_t b) {
    return alu_c_mul_op<16, false, true >(a, b);
}
uint32_t core::alu_c_mulh8(uint32_t a, uint32_t b) {
    return alu_c_mul_op<8,  true,  true >(a, b);
}
uint32_t core::alu_c_mulh8u(uint32_t a, uint32_t b) {
    return alu_c_mul_op<8,  false, true >(a, b);
}

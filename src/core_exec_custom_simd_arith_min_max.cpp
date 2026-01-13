#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

// covers: min/max, signed/unsigned, any bit width
template <int vbits, bool vsigned, alu_min_max_op_t op>
uint32_t core::alu_c_min_max_op(uint32_t a, uint32_t b) {
    constexpr int e = (32 / vbits);
    constexpr uint32_t mask = ((1U << vbits) - 1);
    uint32_t res_packed = 0;

    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (int i = 0; i < e; i++) {
        int32_t val_a = extract_val<vbits, vsigned>(a);
        int32_t val_b = extract_val<vbits, vsigned>(b);
        int32_t res;
        if constexpr (op == alu_min_max_op_t::min) res = std::min(val_a, val_b);
        else res = std::max(val_a, val_b);

        #ifdef DASM_EN
        simd_ss_append(static_cast<int32_t>(res), val_a, val_b);
        #endif

        res_packed |= (static_cast<uint32_t>(res) & mask) << (i * vbits);

        // shift inputs for next iteration
        a >>= vbits;
        b >>= vbits;
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", "]");
    #endif

    return res_packed;
}

uint32_t core::alu_c_min16(uint32_t a, uint32_t b) {
    return alu_c_min_max_op<16, true, alu_min_max_op_t::min>(a, b);
}

uint32_t core::alu_c_min16u(uint32_t a, uint32_t b) {
    return alu_c_min_max_op<16, false, alu_min_max_op_t::min>(a, b);
}

uint32_t core::alu_c_min8(uint32_t a, uint32_t b) {
    return alu_c_min_max_op<8, true, alu_min_max_op_t::min>(a, b);
}

uint32_t core::alu_c_min8u(uint32_t a, uint32_t b) {
    return alu_c_min_max_op<8, false, alu_min_max_op_t::min>(a, b);
}

uint32_t core::alu_c_max16(uint32_t a, uint32_t b) {
    return alu_c_min_max_op<16, true, alu_min_max_op_t::max>(a, b);
}

uint32_t core::alu_c_max16u(uint32_t a, uint32_t b) {
    return alu_c_min_max_op<16, false, alu_min_max_op_t::max>(a, b);
}

uint32_t core::alu_c_max8(uint32_t a, uint32_t b) {
    return alu_c_min_max_op<8, true, alu_min_max_op_t::max>(a, b);
}

uint32_t core::alu_c_max8u(uint32_t a, uint32_t b) {
    return alu_c_min_max_op<8, false, alu_min_max_op_t::max>(a, b);
}

#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

template <size_t vbits, bool vsigned>
constexpr int32_t get_limit(bool max) {
    if constexpr (vsigned) {
        // e.g. 8-bit signed: max 127, min -128
        return max ? ((1 << (vbits - 1)) - 1) : -(1 << (vbits - 1));
    } else {
        // e.g. 8-bit unsigned: max 255, min 0
        return max ? ((1 << vbits) - 1) : 0;
    }
}

// covers: add/sub, signed/unsigned, saturating/wrapping, any bit width
template <size_t vbits, bool vsigned, alu_add_sub_op_t op, bool sat>
uint32_t core::alu_c_add_sub_op(uint32_t a, uint32_t b) {
    constexpr size_t e = (32 / vbits);
    constexpr uint32_t mask = ((1U << vbits) - 1);
    constexpr int32_t max_val = get_limit<vbits, vsigned>(true);
    constexpr int32_t min_val = get_limit<vbits, vsigned>(false);
    uint32_t res_packed = 0;

    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t val_a = extract_val<vbits, vsigned>(a);
        int32_t val_b = extract_val<vbits, vsigned>(b);

        // in 32-bit wide space to catch overflows
        int32_t raw_res;
        if constexpr (op == alu_add_sub_op_t::add) raw_res = (val_a + val_b);
        else raw_res = (val_a - val_b);

        // saturation or wrapping
        int32_t final_val;
        if constexpr (sat) {
            if (raw_res > max_val) final_val = max_val;
            else if (raw_res < min_val) final_val = min_val;
            else final_val = raw_res;
        } else {
            final_val = raw_res; // standard wrap-around
        }

        #ifdef DASM_EN
        simd_ss_append(static_cast<int32_t>(final_val), val_a, val_b);
        #endif

        res_packed |= (static_cast<uint32_t>(final_val) & mask) << (i * vbits);

        // shift inputs for next iteration
        a >>= vbits;
        b >>= vbits;
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", "]");
    #endif

    return res_packed;
}

// wrap
uint32_t core::alu_c_add16(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<16, true, alu_add_sub_op_t::add, false>(a, b);
}

uint32_t core::alu_c_add8(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<8, true, alu_add_sub_op_t::add, false>(a, b);
}

uint32_t core::alu_c_sub16(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<16, true, alu_add_sub_op_t::sub, false>(a, b);
}

uint32_t core::alu_c_sub8(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<8, true, alu_add_sub_op_t::sub, false>(a, b);
}

// saturating
uint32_t core::alu_c_qadd16(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<16, true, alu_add_sub_op_t::add, true>(a, b);
}

uint32_t core::alu_c_qadd16u(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<16, false, alu_add_sub_op_t::add, true>(a, b);
}

uint32_t core::alu_c_qadd8(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<8, true, alu_add_sub_op_t::add, true>(a, b);
}

uint32_t core::alu_c_qadd8u(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<8, false, alu_add_sub_op_t::add, true>(a, b);
}

uint32_t core::alu_c_qsub16(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<16, true, alu_add_sub_op_t::sub, true>(a, b);
}

uint32_t core::alu_c_qsub16u(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<16, false, alu_add_sub_op_t::sub, true>(a, b);
}

uint32_t core::alu_c_qsub8(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<8, true, alu_add_sub_op_t::sub, true>(a, b);
}

uint32_t core::alu_c_qsub8u(uint32_t a, uint32_t b) {
    return alu_c_add_sub_op<8, false, alu_add_sub_op_t::sub, true>(a, b);
}

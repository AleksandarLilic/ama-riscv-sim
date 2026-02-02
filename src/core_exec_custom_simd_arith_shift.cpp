#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

// covers: left/right, logic/arith, any bit width
template <size_t vbits, bool varith, alu_shift_op_t op>
uint32_t core::alu_c_shift_op(uint32_t a, uint32_t shamt) {
    constexpr size_t e = (32 / vbits);
    constexpr uint32_t mask = ((1U << vbits) - 1);
    uint32_t res_packed = 0;

    #ifdef DASM_EN
    simd_ss_init_ca();
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t val_a = extract_val<vbits, varith>(a);
        int32_t res;
        if constexpr (op == alu_shift_op_t::left) res = (val_a << shamt);
        else res = (val_a >> shamt);

        #ifdef DASM_EN
        simd_ss_append_imm((res & mask), val_a, (vbits >> 2));
        #endif

        res_packed |= (TO_U32(res) & mask) << (i * vbits);

        // shift inputs for next iteration
        a >>= vbits;
    }

    #ifdef DASM_EN
    simd_ss_finish_cas(shamt);
    #endif

    return res_packed;
}

uint32_t core::alu_c_slli16(uint32_t a, uint32_t shamt) {
    return alu_c_shift_op<16, false, alu_shift_op_t::left>(a, shamt);
}

uint32_t core::alu_c_slli8(uint32_t a, uint32_t shamt) {
    return alu_c_shift_op<8, false, alu_shift_op_t::left>(a, shamt);
}

uint32_t core::alu_c_srli16(uint32_t a, uint32_t shamt) {
    return alu_c_shift_op<16, false, alu_shift_op_t::right>(a, shamt);
}

uint32_t core::alu_c_srli8(uint32_t a, uint32_t shamt) {
    return alu_c_shift_op<8, false, alu_shift_op_t::right>(a, shamt);
}

uint32_t core::alu_c_srai16(uint32_t a, uint32_t shamt) {
    return alu_c_shift_op<16, true, alu_shift_op_t::right>(a, shamt);
}

uint32_t core::alu_c_srai8(uint32_t a, uint32_t shamt) {
    return alu_c_shift_op<8, true, alu_shift_op_t::right>(a, shamt);
}

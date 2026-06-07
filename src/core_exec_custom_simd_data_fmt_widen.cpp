#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

template <size_t vbits, bool vsigned>
reg_pair core::data_fmt_c_widen_t(uint32_t a, uint32_t shamt) {
    // widen n-bit to 2n-bit elements
    constexpr size_t e = lane<vbits>::count;
    constexpr size_t out_bits = (vbits * 2);
    shamt &= (out_bits - 1); // shift is in the widened (out_bits) domain

    int32_t vals[e];
    #ifdef DASM_EN
    constexpr size_t half_e = (e / 2);
    simd_ss_init_ca();
    #endif

    for (size_t i = 0; i < e; i++) {
        vals[i] = extract_val<vbits, vsigned>(a);
        vals[i] <<= shamt;
        #ifdef DASM_EN
        simd_ss_append_a(vals[i]);
        #endif
        a >>= vbits;
    }

    #ifdef DASM_EN
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << vals[i];
        if (i == (half_e - 1)) dasm.simd_c << " ], [ "; // separator at rd/rdp
        else dasm.simd_c << " ";
    }
    simd_ss_finish_cas(shamt);
    #endif

    return pack_wide<out_bits>(vals);
}

reg_pair core::data_fmt_c_widen16(uint32_t a, uint32_t shamt) {
    return data_fmt_c_widen_t<16, true>(a, shamt);
}

reg_pair core::data_fmt_c_widen16u(uint32_t a, uint32_t shamt) {
    return data_fmt_c_widen_t<16, false>(a, shamt);
}

reg_pair core::data_fmt_c_widen8(uint32_t a, uint32_t shamt) {
    return data_fmt_c_widen_t<8, true>(a, shamt);
}

reg_pair core::data_fmt_c_widen8u(uint32_t a, uint32_t shamt) {
    return data_fmt_c_widen_t<8, false>(a, shamt);
}

reg_pair core::data_fmt_c_widen4(uint32_t a, uint32_t shamt) {
    return data_fmt_c_widen_t<4, true>(a, shamt);
}

reg_pair core::data_fmt_c_widen4u(uint32_t a, uint32_t shamt) {
    return data_fmt_c_widen_t<4, false>(a, shamt);
}

reg_pair core::data_fmt_c_widen2(uint32_t a, uint32_t shamt) {
    return data_fmt_c_widen_t<2, true>(a, shamt);
}

reg_pair core::data_fmt_c_widen2u(uint32_t a, uint32_t shamt) {
    return data_fmt_c_widen_t<2, false>(a, shamt);
}

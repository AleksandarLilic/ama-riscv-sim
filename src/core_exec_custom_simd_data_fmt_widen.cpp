#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

template <size_t vbits, bool vsigned>
reg_pair core::data_fmt_c_widen_t(uint32_t a, uint32_t shamt) {
    // widen n-bit to 2n-bit elements
    constexpr size_t e = (32 / vbits);
    constexpr size_t s = vbits;
    constexpr size_t out_bits = (vbits * 2);
    constexpr size_t half_e = (e / 2);
    constexpr uint32_t mask = ((1ULL << out_bits) - 1);

    int32_t vals[e];
    #ifdef DASM_EN
    simd_ss_init_cab(std::to_string(shamt));
    #endif

    for (size_t i = 0; i < e; i++) {
        vals[i] = extract_val<vbits, vsigned>(a);
        vals[i] <<= (shamt & mask);
        #ifdef DASM_EN
        simd_ss_append_a(vals[i]);
        #endif
        a >>= s;
    }

    #ifdef DASM_EN
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << vals[i];
        if (i == (half_e - 1)) dasm.simd_c << " ], [ "; // separator at rd/rdp
        else dasm.simd_c << " ";
    }
    simd_ss_finish_cai();
    #endif

    uint32_t words[2] = {0, 0};

    for (size_t i = 0; i < e; i++) {
        size_t w_idx = (i / half_e);
        size_t shift = ((i % half_e) * out_bits);
        words[w_idx] |= (TO_U32(vals[i]) & mask) << shift;
    }

    return {words[0], words[1]};
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

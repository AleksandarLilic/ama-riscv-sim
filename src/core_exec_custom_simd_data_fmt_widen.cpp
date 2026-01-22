#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

template <int vbits, bool vsigned>
reg_pair core::data_fmt_c_widen_t(uint32_t a) {
    // widen n-bit to 2n-bit elements
    constexpr size_t e = (32 / vbits);
    constexpr size_t s = vbits;
    constexpr size_t out_bits = (vbits * 2);
    constexpr size_t half_e = (e / 2);

    int32_t vals[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        vals[i] = extract_val<vbits, vsigned>(a);
        #ifdef DASM_EN
        simd_ss_append(vals[i]);
        #endif
        a >>= s;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ ";
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << vals[i];
        // add separators at the split point and the end
        if (i == (half_e - 1)) dasm.simd_c << " ], [ ";
        else if (i == (e - 1)) dasm.simd_c << " ]";
        else dasm.simd_c << " ";
    }
    simd_ss_finish("");
    #endif

    uint32_t words[2] = {0, 0};
    constexpr uint32_t mask = ((1ULL << out_bits) - 1);

    for (size_t i = 0; i < e; i++) {
        size_t w_idx = (i / half_e);
        size_t shift = ((i % half_e) * out_bits);
        words[w_idx] |= (static_cast<uint32_t>(vals[i]) & mask) << shift;
    }

    return {words[0], words[1]};
}

reg_pair core::data_fmt_c_widen16(uint32_t a) {
    return data_fmt_c_widen_t<16, true>(a);
}

reg_pair core::data_fmt_c_widen16u(uint32_t a) {
    return data_fmt_c_widen_t<16, false>(a);
}

reg_pair core::data_fmt_c_widen8(uint32_t a) {
    return data_fmt_c_widen_t<8, true>(a);
}

reg_pair core::data_fmt_c_widen8u(uint32_t a) {
    return data_fmt_c_widen_t<8, false>(a);
}

reg_pair core::data_fmt_c_widen4(uint32_t a) {
    return data_fmt_c_widen_t<4, true>(a);
}

reg_pair core::data_fmt_c_widen4u(uint32_t a) {
    return data_fmt_c_widen_t<4, false>(a);
}

reg_pair core::data_fmt_c_widen2(uint32_t a) {
    return data_fmt_c_widen_t<2, true>(a);
}

reg_pair core::data_fmt_c_widen2u(uint32_t a) {
    return data_fmt_c_widen_t<2, false>(a);
}

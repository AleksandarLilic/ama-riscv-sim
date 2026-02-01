#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

template <size_t vbits, bool vsat, bool vsigned>
uint32_t core::data_fmt_c_narrow_t(uint32_t a, uint32_t b) {
    // narrow 2 registers of n-bit elements into 1 register of n/2-bit elements
    // supports both truncation (vsat=false) and saturation (vsat=true)
    constexpr size_t out_bits = (vbits / 2);
    constexpr size_t e = (32 / vbits); // elements per input reg
    #ifdef DASM_EN
    constexpr size_t out_e = (e * 2); // total elements in output
    #endif
    constexpr uint32_t out_mask = ((1ULL << out_bits) - 1);

    // saturation bounds, optimized out if unused
    int32_t max_val, min_val;
    if constexpr (vsigned) {
        max_val = ((1 << (out_bits - 1)) - 1);
        min_val = (-(1 << (out_bits - 1)));
    } else {
        max_val = (1 << out_bits) - 1;
        min_val = 0;
    }

    #ifdef DASM_EN
    int32_t out_vals[out_e];
    #endif
    uint32_t res = 0;

    // helper lambda to process a single register's worth of elements
    auto process_reg = [&](uint32_t val, size_t offset) {
        for (size_t i = 0; i < e; i++) {
            int32_t raw = extract_val<vbits, vsigned>(val);

            // saturate?
            if constexpr (vsat) {
                if constexpr (vsigned) {
                    if (raw > max_val) raw = max_val;
                    else if (raw < min_val) raw = min_val;
                } else {
                    if (TO_U32(raw) > TO_U32(max_val)) raw = max_val;
                    else if (TO_U32(raw) < TO_U32(min_val)) raw = min_val;
                }
            }

            #ifdef DASM_EN
            int32_t m_val = (raw & out_mask);
            if constexpr (vsigned && vsat) {
                constexpr int32_t s = (32 - out_bits);
                out_vals[offset + i] = ((m_val << s) >> s);
            } else {
                out_vals[offset + i] = m_val;
            }
            #endif

            // pack masked value into result
            // offset + i determines position in the final 32-bit word
            res |= (TO_U32(raw) & out_mask) << ((offset + i) * out_bits);

            if constexpr (vbits < 32) val >>= vbits;
        }
    };

    #ifdef DASM_EN
    simd_ss_init_cab();
    #endif

    process_reg(a, 0);
    process_reg(b, e);

    #ifdef DASM_EN
    for (size_t i = 0; i < e; i++) {
        if constexpr (vsigned) {
            simd_ss_append_ab(
                extract_val<vbits, vsigned>(a >> (i * vbits)),
                extract_val<vbits, vsigned>(b >> (i * vbits))
            );
        } else {
            simd_ss_append_ab_u(
                TO_U32((extract_val<vbits, vsigned>(a >> (i * vbits)))),
                TO_U32((extract_val<vbits, vsigned>(b >> (i * vbits))))
            );
        }
    }

    for (size_t i = 0; i < out_e; i++) dasm.simd_c << out_vals[i] << " ";
    simd_ss_finish_cab();
    #endif

    return res;
}

// narrow truncating
uint32_t core::data_fmt_c_narrow32(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<32, false, false>(a, b);
}

uint32_t core::data_fmt_c_narrow16(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<16, false, false>(a, b);
}

uint32_t core::data_fmt_c_narrow8(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<8, false, false>(a, b);
}

uint32_t core::data_fmt_c_narrow4(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<4, false, false>(a, b);
}

// narrow saturating
uint32_t core::data_fmt_c_qnarrow32(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<32, true, true>(a, b);
}

uint32_t core::data_fmt_c_qnarrow32u(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<32, true, false>(a, b);
}

uint32_t core::data_fmt_c_qnarrow16(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<16, true, true>(a, b);
}

uint32_t core::data_fmt_c_qnarrow16u(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<16, true, false>(a, b);
}

uint32_t core::data_fmt_c_qnarrow8(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<8, true, true>(a, b);
}

uint32_t core::data_fmt_c_qnarrow8u(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<8, true, false>(a, b);
}

uint32_t core::data_fmt_c_qnarrow4(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<4, true, true>(a, b);
}

uint32_t core::data_fmt_c_qnarrow4u(uint32_t a, uint32_t b) {
    return data_fmt_c_narrow_t<4, true, false>(a, b);
}

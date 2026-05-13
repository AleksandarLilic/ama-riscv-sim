#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

template <size_t vbits, bool vsigned>
reg_pair core::alu_c_wmul_op(uint32_t a, uint32_t b) {
    // multiply n-bit elements to 2n-bit results
    // e.g., 16-bit * 16-bit -> 32-bit result
    constexpr size_t e = lane<vbits>::count;
    constexpr size_t out_bits = (vbits * 2);

    int32_t results[e];

    #ifdef DASM_EN
    constexpr size_t half_e = (e / 2);
    simd_ss_init_cab();
    #endif

    // extract inputs and multiply
    for (size_t i = 0; i < e; i++) {
        int32_t val_a = extract_val<vbits, vsigned>(a);
        int32_t val_b = extract_val<vbits, vsigned>(b);

        // standard 32-bit multiply is sufficient for max 16x16 case
        results[i] = (val_a * val_b);

        #ifdef DASM_EN
        simd_ss_append_ab(val_a, val_b);
        #endif

        a >>= vbits;
        b >>= vbits;
    }

    #ifdef DASM_EN
    // format the result string: [ r0 r1 ], [ r2 r3 ]

    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << results[i];
        // add separators at the split point (between reg words) and end
        if (i == half_e - 1) dasm.simd_c << " ], [ ";
        else dasm.simd_c << " ";
    }
    simd_ss_finish_cab();
    #endif

    return pack_wide<out_bits>(results);
}

// instantiations
reg_pair core::alu_c_wmul16(uint32_t a, uint32_t b) {
    return alu_c_wmul_op<16, true>(a, b);
}

reg_pair core::alu_c_wmul16u(uint32_t a, uint32_t b) {
    return alu_c_wmul_op<16, false>(a, b);
}

reg_pair core::alu_c_wmul8(uint32_t a, uint32_t b) {
    return alu_c_wmul_op<8, true>(a, b);
}

reg_pair core::alu_c_wmul8u(uint32_t a, uint32_t b) {
    return alu_c_wmul_op<8, false>(a, b);
}

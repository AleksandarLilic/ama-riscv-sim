#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

template <size_t vbits, bool vsigned>
reg_pair core::alu_c_wmul_op(uint32_t a, uint32_t b) {
    // multiply n-bit elements to 2n-bit results
    // e.g., 16-bit * 16-bit -> 32-bit result
    constexpr size_t e = (32 / vbits);
    constexpr size_t out_bits = (vbits * 2);
    constexpr size_t half_e = (e / 2);
    constexpr uint32_t mask = ((1ULL << out_bits) - 1);

    int32_t results[e];

    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    // extract inputs and multiply
    for (size_t i = 0; i < e; i++) {
        int32_t val_a = extract_val<vbits, vsigned>(a);
        int32_t val_b = extract_val<vbits, vsigned>(b);

        // standard 32-bit multiply is sufficient for max 16x16 case
        results[i] = (val_a * val_b);

        #ifdef DASM_EN
        simd_ss_append(val_a, val_b);
        #endif

        a >>= vbits;
        b >>= vbits;
    }

    #ifdef DASM_EN
    // format the result string: [ r0 r1 ], [ r2 r3 ]
    dasm.simd_a << "]";
    dasm.simd_b << "]";

    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << results[i];
        // add separators at the split point (between reg words) and end
        if (i == half_e - 1) dasm.simd_c << " ], [ ";
        else if (i == e - 1) dasm.simd_c << " ]";
        else dasm.simd_c << " ";
    }
    simd_ss_finish("", "", "");
    #endif

    //  pack results into two 32-bit words
    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < e; i++) {
        // determine which output word (0 or 1) gets this element
        size_t w_idx = (i / half_e);
        // calculate bit offset within that word
        size_t shift = ((i % half_e) * out_bits);
        // mask result to output width and shift into place
        words[w_idx] |= ((TO_U32(results[i]) & mask) << shift);
    }

    return {words[0], words[1]};
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

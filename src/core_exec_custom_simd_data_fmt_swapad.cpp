#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

template <size_t vbits>
reg_pair core::data_fmt_c_swapad_t(uint32_t a, uint32_t b) {
    // swapad spec: rd = [rs1[0], rs2[0], rs1[2], rs2[2], ...],
    //              rdp = [rs1[1], rs2[1], rs1[3], rs2[3], ...]
    // rd gets even lanes interleaved rs1/rs2
    // rdp gets odd lanes interleaved rs1/rs2
    constexpr size_t e = (32 / vbits);
    constexpr uint32_t mask = ((1U << vbits) - 1);
    constexpr size_t half_e = (e / 2);

    uint32_t rd_val = 0;
    uint32_t rdp_val = 0;

    #ifdef DASM_EN
    simd_ss_init_cab();
    #endif

    // one iteration per (even, odd) lane pair:
    // j=0 -> lanes 0,1; j=1 -> 2,3; ...
    for (size_t j = 0; j < half_e; j++) {
        size_t i_even = (2 * j);
        size_t i_odd = (2 * j + 1);

        // extract from rs1 (a) and rs2 (b): even lanes and odd lanes
        uint32_t a_even = ((a >> (i_even * vbits)) & mask);
        uint32_t b_even = ((b >> (i_even * vbits)) & mask);
        uint32_t a_odd = ((a >> (i_odd * vbits)) & mask);
        uint32_t b_odd = ((b >> (i_odd * vbits)) & mask);

        // rd: write [a_even, b_even] at pos i_even, i_even+1
        rd_val |= ((a_even << (i_even * vbits)) | (b_even << (i_odd * vbits)));
        // rdp:, write [a_odd, b_odd] at the same output positions
        rdp_val |= ((a_odd << (i_even * vbits)) | (b_odd << (i_odd * vbits)));

        #ifdef DASM_EN
        simd_ss_append_ab(TO_I32(a_even), TO_I32(b_even));
        simd_ss_append_ab(TO_I32(a_odd), TO_I32(b_odd));
        #endif
    }

    #ifdef DASM_EN
    // simd_c: rd then rdp "[ rd_elems ], [ rdp_elems ]"
    for (size_t j = 0; j < half_e; j++) {
        uint32_t a_even = ((a >> (2 * j * vbits)) & mask);
        uint32_t b_even = ((b >> (2 * j * vbits)) & mask);
        dasm.simd_c << TO_I32(a_even) << " " << TO_I32(b_even) << " ";
    }

    dasm.simd_c << "], [ ";
    for (size_t j = 0; j < half_e; j++) {
        uint32_t a_odd = ((a >> ((2 * j + 1) * vbits)) & mask);
        uint32_t b_odd = ((b >> ((2 * j + 1) * vbits)) & mask);
        dasm.simd_c << TO_I32(a_odd) << " " << TO_I32(b_odd) << " ";
    }

    simd_ss_finish_cab();
    #endif

    return {rd_val, rdp_val};
}

reg_pair core::data_fmt_c_swapad16(uint32_t a, uint32_t b) {
    return data_fmt_c_swapad_t<16>(a, b);
}

reg_pair core::data_fmt_c_swapad8(uint32_t a, uint32_t b) {
    return data_fmt_c_swapad_t<8>(a, b);
}

reg_pair core::data_fmt_c_swapad4(uint32_t a, uint32_t b) {
    return data_fmt_c_swapad_t<4>(a, b);
}

reg_pair core::data_fmt_c_swapad2(uint32_t a, uint32_t b) {
    return data_fmt_c_swapad_t<2>(a, b);
}

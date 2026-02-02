#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

template <size_t vbits>
uint32_t core::data_fmt_c_dup_t(uint32_t rs1) {
    // dup: broadcast low vbits of rs1 (sign-extended) into all lanes
    // rd.H[x] = int16_t(rs1) for x=0..LEN-1
    constexpr size_t e = (32 / vbits);
    constexpr uint32_t mask = ((1U << vbits) - 1);

    int32_t scalar = extract_val<vbits, true>(rs1);
    uint32_t lane_val = TO_U32(scalar) & mask;
    uint32_t res = 0;

    #ifdef DASM_EN
    simd_ss_init_c();
    #endif

    for (size_t i = 0; i < e; i++) {
        res |= lane_val << (i * vbits);
        #ifdef DASM_EN
        simd_ss_append_c(scalar);
        #endif
    }

    #ifdef DASM_EN
    simd_ss_finish_dup(scalar);
    #endif

    return res;
}

uint32_t core::data_fmt_c_dup16(uint32_t rs1) {
    return data_fmt_c_dup_t<16>(rs1);
}

uint32_t core::data_fmt_c_dup8(uint32_t rs1) {
    return data_fmt_c_dup_t<8>(rs1);
}

uint32_t core::data_fmt_c_dup4(uint32_t rs1) {
    return data_fmt_c_dup_t<4>(rs1);
}

uint32_t core::data_fmt_c_dup2(uint32_t rs1) {
    return data_fmt_c_dup_t<2>(rs1);
}

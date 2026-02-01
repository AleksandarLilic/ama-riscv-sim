#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

// vins: insert low vbits of rs1 into lane at idx of rd (RMW)
template <size_t vbits>
uint32_t core::data_fmt_c_vins_t(uint32_t rd, uint32_t rs1, uint8_t idx) {
    constexpr size_t lanes = (32 / vbits);
    constexpr uint32_t lane_mask_bits = ((1U << vbits) - 1);
    idx = TO_U8(idx & (lanes - 1));
    uint32_t clear_lane = (~(lane_mask_bits << (idx * vbits)));
    uint32_t insert = ((rs1 & lane_mask_bits) << (idx * vbits));
    uint32_t res = ((rd & clear_lane) | insert);

    #ifdef DASM_EN
    // RD = result vector; RS1 = scalar (inserted value); RS2 = lane index
    simd_ss_init_vs_ab();
    for (size_t i = 0; i < lanes; i++) {
        simd_ss_append_c(extract_val<vbits, true>(res >> (i * vbits)));
    }
    simd_ss_finish_vs_ab(
        extract_val<vbits, true>(rs1), static_cast<int32_t>(idx));
    #endif

    return res;
}

uint32_t core::data_fmt_c_vins16(uint32_t rd, uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vins_t<16>(rd, rs1, idx);
}

uint32_t core::data_fmt_c_vins8(uint32_t rd, uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vins_t<8>(rd, rs1, idx);
}

uint32_t core::data_fmt_c_vins4(uint32_t rd, uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vins_t<4>(rd, rs1, idx);
}

uint32_t core::data_fmt_c_vins2(uint32_t rd, uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vins_t<2>(rd, rs1, idx);
}

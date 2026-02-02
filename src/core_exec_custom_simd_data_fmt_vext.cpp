#include "defines.h"
#include "core.h"
#include "core_exec_custom_simd.h"

// vext: extract lane at idx from rs1, sign/zero extend to 32-bit scalar
template <size_t vbits, bool vsigned>
uint32_t core::data_fmt_c_vext_t(uint32_t rs1, uint8_t idx) {
    constexpr size_t lanes = (32 / vbits);
    idx = TO_U8(idx & (lanes - 1));
    uint32_t lane_val = (rs1 >> (idx * vbits));
    int32_t res = extract_val<vbits, vsigned>(lane_val);

    #ifdef DASM_EN
    // RD = scalar result, RS1 = source vector (all elements), RS2 = lane index
    simd_ss_init_a();
    for (size_t i = 0; i < lanes; i++) {
        simd_ss_append_a(extract_val<vbits, true>(rs1 >> (i * vbits)));
    }
    simd_ss_finish_vext(res, static_cast<int32_t>(idx));
    #endif

    return TO_U32(res);
}

uint32_t core::data_fmt_c_vext16(uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vext_t<16, true>(rs1, idx);
}

uint32_t core::data_fmt_c_vext16u(uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vext_t<16, false>(rs1, idx);
}

uint32_t core::data_fmt_c_vext8(uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vext_t<8, true>(rs1, idx);
}

uint32_t core::data_fmt_c_vext8u(uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vext_t<8, false>(rs1, idx);
}

uint32_t core::data_fmt_c_vext4(uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vext_t<4, true>(rs1, idx);
}

uint32_t core::data_fmt_c_vext4u(uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vext_t<4, false>(rs1, idx);
}

uint32_t core::data_fmt_c_vext2(uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vext_t<2, true>(rs1, idx);
}

uint32_t core::data_fmt_c_vext2u(uint32_t rs1, uint8_t idx) {
    return data_fmt_c_vext_t<2, false>(rs1, idx);
}

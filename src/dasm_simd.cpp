#include "defines.h"
#include "core.h"

#ifdef DASM_EN
void core::simd_ss_clear() {
    dasm.simd_c.str("");
    dasm.simd_a.str("");
    dasm.simd_b.str("");
}

void core::simd_ss_init_ab() {
    simd_ss_clear();
    dasm.simd_a << "[ ";
    dasm.simd_b << "[ ";
}

void core::simd_ss_init_ca() {
    simd_ss_clear();
    dasm.simd_c << "[ ";
    dasm.simd_a << "[ ";
}

void core::simd_ss_init_cab() {
    simd_ss_clear();
    dasm.simd_c << "[ ";
    dasm.simd_a << "[ ";
    dasm.simd_b << "[ ";
}

void core::simd_ss_init_c() {
    simd_ss_clear();
    dasm.simd_c << "[ ";
}

void core::simd_ss_init_a() {
    simd_ss_clear();
    dasm.simd_a << "[ ";
}

void core::simd_ss_append_a(int32_t a) {
    dasm.simd_a << a << " ";
}

void core::simd_ss_append_c(int32_t c) {
    dasm.simd_c << c << " ";
}

void core::simd_ss_append_ab(int32_t a, int32_t b) {
    dasm.simd_a << a << " ";
    dasm.simd_b << b << " ";
}

void core::simd_ss_append_ab_u(uint32_t a, uint32_t b) {
    dasm.simd_a << a << " ";
    dasm.simd_b << b << " ";
}

void core::simd_ss_append_cab(int32_t c, int32_t a, int32_t b) {
    dasm.simd_c << c << " ";
    dasm.simd_a << a << " ";
    dasm.simd_b << b << " ";
}

void core::simd_ss_append_imm(int32_t c, int32_t a, size_t w) {
    dasm.simd_c << FHEXZ(c, w) << " ";
    dasm.simd_a << FHEXZ(a, w) << " ";
}

void core::simd_ss_finish_ca() {
    if (!ip.rd()) return;
    dasm.simd_ss << "RD = " << dasm.simd_c.str() << "], "
                 << "RS1 = " << dasm.simd_a.str() << "]; ";
}

void core::simd_ss_finish_cas(int32_t shamt) {
    if (!ip.rd()) return;
    dasm.simd_ss << "RD = " << dasm.simd_c.str() << "], "
                 << "RS1 = " << dasm.simd_a.str() << "], "
                 << "SHAMT = " << shamt << "; ";
}

void core::simd_ss_finish_cab() {
    if (!ip.rd()) return;
    dasm.simd_ss << "RD = " << dasm.simd_c.str() << "], "
                 << "RS1 = " << dasm.simd_a.str() << "], "
                 << "RS2 = " << dasm.simd_b.str() << "]; ";
}

void core::simd_ss_finish_dot(int32_t res, int32_t c_in) {
    if (!ip.rd()) return;
    dasm.simd_ss << "RD = " << res << ", "
                 << "RS1 = " << dasm.simd_a.str() << "], "
                 << "RS2 = " << dasm.simd_b.str() << "]; "
                 << ", RS3 = " << c_in << "; ";
}

void core::simd_ss_finish_vins(int32_t a, int32_t idx) {
    if (!ip.rd()) return;
    dasm.simd_ss << "RD = " << dasm.simd_c.str() << "], "
                 << "RS1 = " << a
                 << ", IDX = " << idx << "; ";
}

void core::simd_ss_finish_vext(int32_t c_scalar, int32_t idx) {
    if (!ip.rd()) return;
    dasm.simd_ss << "RD = " << c_scalar << ", "
                 << "RS1 = " << dasm.simd_a.str() << "], "
                 << "IDX = " << idx << "; ";
}

void core::simd_ss_finish_dup(int32_t a_scalar) {
    if (!ip.rd()) return;
    dasm.simd_ss << "RD = " << dasm.simd_c.str() << "], "
                 << "RS1 = " << a_scalar << "; ";
}

#endif

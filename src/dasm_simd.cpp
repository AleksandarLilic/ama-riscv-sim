#include "defines.h"
#include "core.h"

#ifdef DASM_EN
void core::simd_ss_init(std::string a) {
    if (!ip.rd()) return;
    dasm.simd_a.str("");
    dasm.simd_c.str("");
    dasm.simd_a << a;
}

void core::simd_ss_init(std::string a, std::string b) {
    if (!ip.rd()) return;
    dasm.simd_a.str("");
    dasm.simd_b.str("");
    dasm.simd_a << a;
    dasm.simd_b << b;
}

void core::simd_ss_init(std::string c, std::string a, std::string b) {
    if (!ip.rd()) return;
    simd_ss_init(a, b);
    dasm.simd_c.str("");
    dasm.simd_c << c;
}

void core::simd_ss_append(int32_t a) {
    if (!ip.rd()) return;
    dasm.simd_a << a << " ";
}

void core::simd_ss_append(int32_t a, int32_t b) {
    if (!ip.rd()) return;
    dasm.simd_a << a << " ";
    dasm.simd_b << b << " ";
}

void core::simd_ss_append_u(uint32_t a, uint32_t b) {
    if (!ip.rd()) return;
    dasm.simd_a << a << " ";
    dasm.simd_b << b << " ";
}

void core::simd_ss_append(int32_t c, int32_t a, int32_t b) {
    if (!ip.rd()) return;
    simd_ss_append(a, b);
    dasm.simd_c << c << " ";
}

void core::simd_ss_append_imm(int32_t c, int32_t a, size_t w) {
    if (!ip.rd()) return;
    dasm.simd_c << FHEXZ(c, w) << " ";
    dasm.simd_a << FHEXZ(a, w) << " ";
}

void core::simd_ss_finish(std::string a) {
    if (!ip.rd()) return;
    dasm.simd_a << a;
    dasm.simd_ss << "RD = " << dasm.simd_c.str()
                 << ", RS1 = " << dasm.simd_a.str() << "; ";
}

void core::simd_ss_finish(std::string a, std::string b, int32_t res) {
    if (!ip.rd()) return;
    dasm.simd_a << a;
    dasm.simd_b << b;
    dasm.simd_ss << "RD = " << res
                 << ", RS1 = " << dasm.simd_a.str()
                 << ", RS2 = " << dasm.simd_b.str() << "; ";
}

void core::simd_ss_finish(
    std::string a, std::string b, int32_t res, int32_t rs3) {
    if (!ip.rd()) return;
    dasm.simd_a << a;
    dasm.simd_b << b;
    dasm.simd_ss << "RD = " << res
                 << ", RS1 = " << dasm.simd_a.str()
                 << ", RS2 = " << dasm.simd_b.str()
                 << ", RS3 = " << rs3 << "; ";
}

void core::simd_ss_finish(std::string c, std::string a, std::string b) {
    if (!ip.rd()) return;
    dasm.simd_a << a;
    dasm.simd_b << b;
    dasm.simd_c << c;
    dasm.simd_ss << "RD = " << dasm.simd_c.str()
                 << ", RS1 = " << dasm.simd_a.str()
                 << ", RS2 = " << dasm.simd_b.str() << "; ";
}
#endif

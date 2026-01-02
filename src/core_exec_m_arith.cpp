#include "defines.h"
#include "core.h"

// arithmetic and logic operations - M extension
uint32_t core::alu_mul(uint32_t a, uint32_t b) {
    return TO_I32(a) * TO_I32(b);
};
uint32_t core::alu_mulh(uint32_t a, uint32_t b) {
    int64_t res = TO_I64(TO_I32(a)) * TO_I64(TO_I32(b));
    return res >> 32;
};
uint32_t core::alu_mulhsu(uint32_t a, uint32_t b) {
    int64_t res = TO_I64(TO_I32(a)) * TO_I64(b);
    return res >> 32;
};
uint32_t core::alu_mulhu(uint32_t a, uint32_t b) {
    uint64_t res = TO_U64(a) * TO_U64(b);
    return res >> 32;
};
uint32_t core::alu_div(uint32_t a, uint32_t b) {
    // division by zero
    if (b == 0) return -1;
    // overflow (most negative int divided by -1)
    if (a == 0x80000000 && b == 0xffffffff) return a;
    return TO_I32(a) / TO_I32(b);
};
uint32_t core::alu_divu(uint32_t a, uint32_t b) {
    if (b == 0) return 0xffffffff;
    return a / b;
};
uint32_t core::alu_rem(uint32_t a, uint32_t b) {
    if (b == 0) return a;
    if (a == 0x80000000 && b == 0xffffffff) return 0;
    return TO_I32(a) % TO_I32(b);
};
uint32_t core::alu_remu(uint32_t a, uint32_t b) {
    if (b == 0) return a;
    return a % b;
};

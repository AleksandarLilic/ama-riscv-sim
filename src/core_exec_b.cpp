#include "defines.h"
#include "core.h"

// arithmetic and logic operations - Zbb extension partial
uint32_t core::alu_max(uint32_t a, uint32_t b) {
    return TO_I32(a) > TO_I32(b) ? a : b;
}
uint32_t core::alu_maxu(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}
uint32_t core::alu_min(uint32_t a, uint32_t b) {
    return TO_I32(a) < TO_I32(b) ? a : b;
}
uint32_t core::alu_minu(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

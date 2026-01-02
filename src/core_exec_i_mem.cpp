#include "defines.h"
#include "core.h"

// load operations
uint32_t core::load_lb(uint32_t addr) {
    PROF_DMEM(dmem_size_t::lb)
    return TO_U32(TO_I8(mem->rd(addr, 1u)));
}
uint32_t core::load_lh(uint32_t addr) {
    PROF_DMEM(dmem_size_t::lh)
    return TO_U32(TO_I16(mem->rd(addr, 2u)));
}
uint32_t core::load_lw(uint32_t addr) {
    PROF_DMEM(dmem_size_t::lw)
    return mem->rd(addr, 4u);
}
uint32_t core::load_lbu(uint32_t addr) {
    PROF_DMEM(dmem_size_t::lb)
    return mem->rd(addr, 1u);
}
uint32_t core::load_lhu(uint32_t addr) {
    PROF_DMEM(dmem_size_t::lh)
    return mem->rd(addr, 2u);
}

// store operations
void core::store_sb(uint32_t addr, uint32_t data) {
    PROF_DMEM(dmem_size_t::sb)
    mem->wr(addr, data, 1u);
}
void core::store_sh(uint32_t addr, uint32_t data) {
    PROF_DMEM(dmem_size_t::sh)
    mem->wr(addr, data, 2u);
}
void core::store_sw(uint32_t addr, uint32_t data) {
    PROF_DMEM(dmem_size_t::sw)
    mem->wr(addr, data, 4u);
}

#include "defines.h"
#include "core.h"

void core::csr_rw(uint32_t init_val_rs1) { W_CSR(init_val_rs1); }
void core::csr_rs(uint32_t init_val_rs1) {
    if (ip.rs1() == 0) return;
    W_CSR(csr.at(ip.csr_addr()).value | init_val_rs1);
}
void core::csr_rc(uint32_t init_val_rs1) {
    if (ip.rs1() == 0) return;
    W_CSR(csr.at(ip.csr_addr()).value & ~init_val_rs1);
}
void core::csr_rwi() { W_CSR(ip.uimm_csr()); }
void core::csr_rsi() {
    if (ip.rs1() == 0) return;
    W_CSR(csr.at(ip.csr_addr()).value | ip.uimm_csr());
}
void core::csr_rci() {
    if (ip.rs1() == 0) return;
    W_CSR(csr.at(ip.csr_addr()).value & ~ip.uimm_csr());
}

void core::csr_cnt_update(uint16_t csr_addr) {
    // csr will be updated for rs, rc, rsi, rci, if rs1 is non-zero reg/val
    bool csr_updated = (
        (ip.rs1() != 0) ||
        (ip.funct3() == TO_U8(csr_op_t::op_rw)) ||
        (ip.funct3() == TO_U8(csr_op_t::op_rwi))
    );

    // if current inst actually writes to instret, skip it in diff
    uint64_t skip = csr_updated;
    skip &= ((csr_addr == CSR_MINSTRET) || (csr_addr == CSR_MINSTRETH));
    uint64_t inst_elapsed = inst_cnt - inst_cnt_csr;
    inst_cnt_csr = inst_cnt + skip;

    // if current inst actually writes to mcycle, skip this cycle in diff
    skip = csr_updated;
    skip &= ((csr_addr == CSR_MCYCLE) || (csr_addr == CSR_MCYCLEH));
    #ifdef DPI
    uint64_t cycle_elapsed = clk_src.get_cr() - cycle_cnt_csr;
    cycle_cnt_csr = clk_src.get_cr() + skip;
    #else
    uint64_t cycle_elapsed = inst_cnt - cycle_cnt_csr; // inst=cycle in isa sim
    cycle_cnt_csr = inst_cnt + skip;
    #endif

    csr.at(CSR_MINSTRET).value += (inst_elapsed & 0xFFFFFFFF);
    csr.at(CSR_MINSTRETH).value += ((inst_elapsed >> 32) & 0xFFFFFFFF);
    csr.at(CSR_MCYCLE).value += (cycle_elapsed & 0xFFFFFFFF);
    csr.at(CSR_MCYCLEH).value += ((cycle_elapsed >> 32) & 0xFFFFFFFF);

    // user mode shadows
    csr.at(CSR_CYCLE).value = csr.at(CSR_MCYCLE).value;
    csr.at(CSR_CYCLEH).value = csr.at(CSR_MCYCLEH).value;
    csr.at(CSR_INSTRET).value = csr.at(CSR_MINSTRET).value;
    csr.at(CSR_INSTRETH).value = csr.at(CSR_MINSTRETH).value;

    #ifdef DPI
    uint64_t mtime_shadow;
    csr_sync_t csr;
    cosim_sync_csrs(&csr);
    mtime_shadow = csr.mtime;
    csr_wide_assign(CSR_MHPMCOUNTER3, csr.mhpmcounter[3]);
    csr_wide_assign(CSR_MHPMCOUNTER4, csr.mhpmcounter[4]);
    csr_wide_assign(CSR_MHPMCOUNTER5, csr.mhpmcounter[5]);
    csr_wide_assign(CSR_MHPMCOUNTER6, csr.mhpmcounter[6]);
    csr_wide_assign(CSR_MHPMCOUNTER7, csr.mhpmcounter[7]);
    csr_wide_assign(CSR_MHPMCOUNTER8, csr.mhpmcounter[8]);
    #else
    uint64_t mtime_shadow = mem->get_mtime_shadow();
    // no pref counters sync, they don't exist in ISA sim
    #endif
    csr_wide_assign(CSR_TIME, mtime_shadow);
}

#include "defines.h"
#include "core.h"

void core::csr_rw(uint32_t init_val_rs1) { W_CSR(init_val_rs1); }

void core::csr_rs(uint32_t init_val_rs1) {
    if (ip.rs1() == 0) return;
    W_CSR(csr.at(TO_U16(ip.csr_addr())).value | init_val_rs1);
}

void core::csr_rc(uint32_t init_val_rs1) {
    if (ip.rs1() == 0) return;
    W_CSR(csr.at(TO_U16(ip.csr_addr())).value & ~init_val_rs1);
}

void core::csr_rwi() { W_CSR(ip.uimm_csr()); }

void core::csr_rsi() {
    if (ip.rs1() == 0) return;
    W_CSR(csr.at(TO_U16(ip.csr_addr())).value | ip.uimm_csr());
}

void core::csr_rci() {
    if (ip.rs1() == 0) return;
    W_CSR(csr.at(TO_U16(ip.csr_addr())).value & ~ip.uimm_csr());
}

void core::csr_cnt_update(uint16_t addr) {
    namespace m = ::csr_map;
    // csr will be updated for rs, rc, rsi, rci, if rs1 is non-zero reg/val
    bool csr_updated = (
        (ip.rs1() != 0) ||
        (ip.funct3() == TO_U8(csr_op_t::op_rw)) ||
        (ip.funct3() == TO_U8(csr_op_t::op_rwi))
    );

    // if current inst actually writes to instret, skip it in diff
    uint64_t skip = csr_updated;
    skip &= ((addr == m::addr::minstret) || (addr == m::addr::minstreth));
    uint64_t inst_elapsed = (sim_cnt.inst - csr_cnt.inst);
    csr_cnt.inst = (sim_cnt.inst + skip);

    // if current inst actually writes to mcycle, skip this cycle in diff
    skip = csr_updated;
    skip &= ((addr == m::addr::mcycle) || (addr == m::addr::mcycleh));
    // inst=cycle in isa sim
    uint64_t cycle_elapsed = (sim_cnt.step - csr_cnt.step);
    csr_cnt.step = (sim_cnt.step + skip);

    csr_wide_add(m::addr::minstret, inst_elapsed);
    csr_wide_add(m::addr::mcycle, cycle_elapsed);

    // user mode shadows
    for (auto &c : csr) {
        if (c.second.perm != csr_def::perm_t::ro_u_shadow) continue;
        if (c.second.s_addr == 0x0) continue; // skip mmio shadows
        c.second.value = csr.at(c.second.s_addr).value;
    }

    // mmio shadows
    csr_wide_assign(m::addr::time, mem->get_mtime_shadow());

    // no perf counters update, events don't exist in the standalone ISA sim
}

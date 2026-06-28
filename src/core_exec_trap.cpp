#include "defines.h"
#include "core.h"

void core::trap_state_update(uint32_t cause, uint32_t tval) {
    csr.at(csr_map::addr::mcause).value = cause;
    csr.at(csr_map::addr::mtval).value = tval; // trap-specific, for trap handler
    csr.at(csr_map::addr::mepc).value = pc; // pc that caused the trap
    csr.at(csr_map::addr::mstatus).value = (
        // clear mpie
        (csr.at(csr_map::addr::mstatus).value & ~csr_map::mstatus::mpie) |
        // mpie = mie
        ((csr.at(csr_map::addr::mstatus).value & csr_map::mstatus::mie) << 4)
    );
    // disable interrupts
    csr.at(csr_map::addr::mstatus).value &= ~csr_map::mstatus::mie;
    pc = csr.at(csr_map::addr::mtvec).value;
    #ifdef PROFILERS_EN
    prof_perf.update_jalr(pc, false, false, 0u);
    #endif
}

void core::mret_state_update() {
    // restore previous interrupt enable bit state
    csr.at(csr_map::addr::mstatus).value = (
        // clear mie
        (csr.at(csr_map::addr::mstatus).value & ~csr_map::mstatus::mie) |
        // mie = mpie
        ((csr.at(csr_map::addr::mstatus).value & csr_map::mstatus::mpie) >> 4) |
        // set mpie
        (csr.at(csr_map::addr::mstatus).value | csr_map::mstatus::mpie)
    );
    // restore pc
    next_pc = csr.at(csr_map::addr::mepc).value;
}

void core::trap_state_update_cb(void* ctx, uint32_t cause, uint32_t tval) {
    static_cast<core*>(ctx)->trap_state_update(cause, tval);
}

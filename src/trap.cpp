#include "trap.h"

#define DASM_PTR_TRAP \
    dasm->asm_ss.str(""); \
    dasm->asm_ss << "Instruction trapped: "
#define FMT_P(x) msg << "> @ " << FORMAT_INST(*pc, *inst, x)
#define FMT FMT_P(8)
#define FMT_ADDR \
    MEM_ADDR_FORMAT(address) << "> with <" << msg << "> @ " \
    << FORMAT_INST(*pc, *inst, 8)

void trap::trap_inst(uint32_t cause, uint32_t tval) {
    std::cout << "; cause=" << std::hex << cause
              << ", tval=" << tval
              << ", pc="  << *pc << std::dec << "\n";
    inst_trapped = true;
    csr->at(CSR_MCAUSE).value = cause;
    csr->at(CSR_MTVAL).value = tval; // trap-specific, for trap handler
    csr->at(CSR_MEPC).value = *pc; // pc that caused the trap
    csr->at(CSR_MSTATUS).value = (
        (csr->at(CSR_MSTATUS).value & ~MSTATUS_MPIE) | // clear mpie
        ((csr->at(CSR_MSTATUS).value & MSTATUS_MIE) << 4) // mpie = mie
    );
    csr->at(CSR_MSTATUS).value &= ~MSTATUS_MIE; // disable interrupts
    *pc = csr->at(CSR_MTVEC).value;
    #ifdef PROFILERS_EN
    prof_perf->update_jalr(*pc, false, false, 0u);
    #endif
}

// exception handling
void trap::e_unsupported_inst(const std::string &msg) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Unsupported instruction <" << FMT;
    #endif
    SIM_TRAP << "Unsupported instruction <" << FMT;
    trap_inst(MCAUSE_ILLEGAL_INST, *inst);
}

void trap::e_illegal_inst(const std::string &msg, uint32_t memw) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Illegal instruction <" << FMT_P(memw);
    #endif
    SIM_TRAP << "Illegal instruction <" << FMT_P(memw);
    trap_inst(MCAUSE_ILLEGAL_INST, *inst);
}

void trap::e_env(const std::string &msg, uint32_t code) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << msg;
    #endif
    SIM_TRAP << msg;
    trap_inst(code, *inst);
}

void trap::e_unsupported_csr(const std::string &msg) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Unsupported instruction <" << FMT;
    #endif
    SIM_TRAP << "Unsupported instruction <" << FMT;
    trap_inst(MCAUSE_ILLEGAL_INST, *inst);
}

void trap::e_dmem_access_fault(
    uint32_t address, const std::string &msg, mem_op_t mem_op) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Memory access fault at address <" << FMT_ADDR;
    #endif
    SIM_TRAP << "Memory access fault at address <" << FMT_ADDR;
    if (mem_op == mem_op_t::read) {
        trap_inst(MCAUSE_LOAD_ACCESS_FAULT, address);
    } else {
        trap_inst(MCAUSE_STORE_ACCESS_FAULT, address);
    }
}

void trap::e_dmem_addr_misaligned(
    uint32_t address, const std::string &msg, mem_op_t mem_op) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Memory misaligned access at address <" << FMT_ADDR;
    #endif
    SIM_TRAP << "Memory misaligned access at address <" << FMT_ADDR;
    if (mem_op == mem_op_t::read) {
        trap_inst(MCAUSE_LOAD_ADDR_MISALIGNED, address);
    } else {
        trap_inst(MCAUSE_STORE_ADDR_MISALIGNED, address);
    }
}

void trap::e_inst_access_fault(
    uint32_t address, const std::string &msg) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Fetch access fault at address <" << FMT_ADDR;
    #endif
    SIM_TRAP << "Fetch access fault at " << FMT_ADDR;
    trap_inst(MCAUSE_INST_ACCESS_FAULT, address);
}

void trap::e_inst_addr_misaligned(
    uint32_t address, const std::string &msg) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Fetch misaligned access at " << FMT_ADDR;
    #endif
    SIM_TRAP << "Fetch misaligned access at " << FMT_ADDR;
    trap_inst(MCAUSE_INST_ADDR_MISALIGNED, address);
}

void trap::e_hardware_error(const std::string &msg) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Hardware Error <" << FMT;
    #endif
    SIM_TRAP << "Hardware Error <" << FMT;
    trap_inst(MCAUSE_HARDWARE_ERROR, *inst);
}

// interrupt handling
void trap::e_timer_interrupt() {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Timer interrupt";
    #endif
    SIM_TRAP << "Timer interrupt";
    trap_inst(MCAUSE_MACHINE_TIMER_INT, 0);
}

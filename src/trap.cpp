#include "trap.h"

#define DASM_PTR_TRAP \
    dasm->asm_ss.str(""); \
    dasm->asm_ss << "Instruction trapped: "

#define FMT_P(x) msg << "> @ " << FORMAT_INST(*pc, *inst, x)
#define FMT FMT_P(8)

#define FMT_ADDR \
    MEM_ADDR_FORMAT(address) << "> with <" << msg << "> @ " \
    << FORMAT_INST(*pc, *inst, 8)

#define TRAP_CAUSE \
    "; cause=" << std::hex << cause \
    << ", tval=" << tval \
    << ", pc="  << *pc << std::dec

void trap::trap_inst(uint32_t cause, uint32_t tval) {
    #ifdef DASM_EN
    dasm->asm_ss << TRAP_CAUSE;
    #endif

    inst_trapped = true;
    csr->at(csrm::addr::mcause).value = cause;
    csr->at(csrm::addr::mtval).value = tval; // trap-specific, for trap handler
    csr->at(csrm::addr::mepc).value = *pc; // pc that caused the trap
    csr->at(csrm::addr::mstatus).value = (
        // clear mpie
        (csr->at(csrm::addr::mstatus).value & ~csrm::mstatus::mpie) |
        // mpie = mie
        ((csr->at(csrm::addr::mstatus).value & csrm::mstatus::mie) << 4)
    );
    // disable interrupts
    csr->at(csrm::addr::mstatus).value &= ~csrm::mstatus::mie;
    *pc = csr->at(csrm::addr::mtvec).value;

    #ifdef PROFILERS_EN
    prof_perf->update_jalr(*pc, false, false, 0u);
    #endif
}

// exception handling
void trap::e_unsupported_inst([[maybe_unused]] const std::string &msg) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Unsupported instruction <" << FMT;
    #endif
    trap_inst(csrm::mcause::illegal_inst, *inst);
}

void trap::e_illegal_inst(
    [[maybe_unused]] const std::string &msg, [[maybe_unused]] uint32_t memw)
{
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Illegal instruction <" << FMT_P(memw);
    #endif
    trap_inst(csrm::mcause::illegal_inst, *inst);
}

void trap::e_env([[maybe_unused]] const std::string &msg, uint32_t code) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << msg;
    #endif
    trap_inst(code, *inst);
}

void trap::e_dmem_access_fault(
    uint32_t address, [[maybe_unused]] const std::string &msg, mem_op_t mem_op)
{
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Memory access fault at address <" << FMT_ADDR;
    #endif
    if (mem_op == mem_op_t::read) {
        trap_inst(csrm::mcause::load_access_fault, address);
    } else {
        trap_inst(csrm::mcause::store_access_fault, address);
    }
}

void trap::e_dmem_addr_misaligned(
    uint32_t address, [[maybe_unused]] const std::string &msg, mem_op_t mem_op)
{
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Memory misaligned access at address <" << FMT_ADDR;
    #endif
    if (mem_op == mem_op_t::read) {
        trap_inst(csrm::mcause::load_addr_misaligned, address);
    } else {
        trap_inst(csrm::mcause::store_addr_misaligned, address);
    }
}

void trap::e_inst_access_fault(
    uint32_t address, [[maybe_unused]] const std::string &msg)
{
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Fetch access fault at address <" << FMT_ADDR;
    #endif
    trap_inst(csrm::mcause::inst_access_fault, address);
}

void trap::e_inst_addr_misaligned(
    uint32_t address, [[maybe_unused]] const std::string &msg)
{
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Fetch misaligned access at " << FMT_ADDR;
    #endif
    trap_inst(csrm::mcause::inst_addr_misaligned, address);
}

void trap::e_hardware_error([[maybe_unused]] const std::string &msg) {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Hardware Error <" << FMT;
    #endif
    trap_inst(csrm::mcause::hardware_error, *inst);
}

// interrupt handling
void trap::e_timer_interrupt() {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "Timer interrupt";
    #endif
    trap_inst(csrm::mcause::intr::machine_timer, 0);
}

void trap::e_external_interrupt() {
    #ifdef DASM_EN
    DASM_PTR_TRAP << "External interrupt";
    #endif
    trap_inst(csrm::mcause::intr::machine_ext, 0);
}

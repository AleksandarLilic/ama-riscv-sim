#pragma once

#include "defines.h"
#include "profiler_perf.h"

class trap {
    private:
        bool inst_trapped = false;
        std::map<uint16_t, CSR>* csr;
        uint32_t* pc;
        uint32_t* inst;
        #ifdef ENABLE_DASM
        dasm_str* dasm;
        #endif
        #ifdef ENABLE_PROF
        profiler_perf* prof_perf;
        #endif

    public:
        trap() = delete;
        trap(std::map<uint16_t, CSR>* csr, uint32_t* pc, uint32_t* inst) :
            csr(csr), pc(pc), inst(inst) {}
        bool is_trapped() { return inst_trapped; }
        void clear_trap() { inst_trapped = false; }
        #ifdef ENABLE_DASM
        void set_dasm(dasm_str* d) { dasm = d; }
        #endif
        #ifdef ENABLE_PROF
        void set_prof_perf(profiler_perf* p) { prof_perf = p; }
        #endif

        // exceptions
        void e_unsupported_inst(const std::string &msg);
        void e_illegal_inst(const std::string &msg, uint32_t memw);
        void e_env(const std::string &msg, uint32_t code);
        void e_unsupported_csr(const std::string &msg);
        void e_dmem_access_fault(
            uint32_t address, const std::string &msg, mem_op_t access);
        void e_dmem_addr_misaligned(
            uint32_t address, const std::string &msg, mem_op_t access);
        void e_inst_access_fault(
            uint32_t address, const std::string &msg);
        void e_inst_addr_misaligned(
            uint32_t address, const std::string &msg);
        void e_hardware_error(const std::string &msg);

        // interrupts
        void e_timer_interrupt();

    private:
        void trap_inst(uint32_t cause, uint32_t tval);
};

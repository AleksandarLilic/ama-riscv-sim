#pragma once

#include "defines.h"
#include "csrs.h"
#include <cassert>
#ifdef PROFILERS_EN
#include "profiler_perf.h"
#endif

class trap {
    private:
        bool inst_trapped = false;
        using trap_state_update_fn_t = void (*)(void*, uint32_t, uint32_t);
        void* trap_state_update_ctx;
        trap_state_update_fn_t fn_ptr_trap_state_update;
        const uint32_t& pc;
        const uint32_t& inst;
        #ifdef DASM_EN
        dasm_str* dasm;
        #endif

    public:
        trap() = delete;
        trap(
            void* ctx,
            trap_state_update_fn_t fn,
            uint32_t& pc,
            uint32_t& inst
        ) :
            trap_state_update_ctx(ctx),
            fn_ptr_trap_state_update(fn),
            pc(pc),
            inst(inst)
        {
            assert(trap_state_update_ctx != nullptr);
            assert(fn_ptr_trap_state_update != nullptr);
        }
        bool is_trapped() { return inst_trapped; }
        void clear_trap() { inst_trapped = false; }
        #ifdef DASM_EN
        void set_dasm(dasm_str* d) { dasm = d; }
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
        void e_external_interrupt();

    private:
        void trap_inst(uint32_t cause, uint32_t tval);
};

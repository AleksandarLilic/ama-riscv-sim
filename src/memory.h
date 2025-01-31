#pragma once

#include "defines.h"
#include "main_memory.h"
#include "trap.h"

#ifdef UART_ENABLE
#include "uart.h"
#endif

struct mem_entry {
    uint32_t base;
    uint32_t size;
    dev* ptr;
};

class memory {
    private:
        main_memory mm;
        #ifdef UART_ENABLE
        uart uart0;
        #endif
        dev *dev_ptr;
        std::array<mem_entry, 2> mem_map;
        trap *tu;
        #ifdef ENABLE_DASM
        dasm_str* dasm;
        #endif

    private:
        void mem_dump(uint32_t start, uint32_t end);
        uint32_t set_addr(uint32_t address, mem_op_t access, uint32_t size);

    public:
        memory() = delete;
        memory(std::string test_elf, hw_cfg_t hw_cfg);
        std::map<uint32_t, symbol_map_entry_t> get_symbol_map() {
            return mm.get_symbol_map();
        }
        void trap_setup(trap* tu) { this->tu = tu; }
        #ifdef ENABLE_DASM
        void set_dasm(dasm_str* d) { dasm = d; }
        #endif
        uint32_t rd_inst(uint32_t address);
        uint32_t rd(uint32_t address, uint32_t size);
        void wr(uint32_t address, uint32_t data, uint32_t size);
        void dump();
        void dump(uint32_t start, uint32_t size);
        scp_status_t cache_hint(uint32_t address, scp_mode_t scp_mode);
        #ifdef ENABLE_HW_PROF
        // propagating to caches
        void log_cache_stats(std::ofstream& log_file) {
            mm.log_cache_stats(log_file);
        }
        void cache_finish() {
            mm.finish();
        }
        void cache_profiling(bool enable) {
            mm.cache_profiling(enable);
        }
        void speculative_exec(speculative_t smode) {
            mm.speculative_exec(smode);
        }
        #ifdef ENABLE_PROF
        void set_perf_profiler(profiler_perf* prof_perf) {
            mm.set_perf_profiler(prof_perf);
        }
        #endif
        #endif
};

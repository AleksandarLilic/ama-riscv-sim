#pragma once

#include "defines.h"
#include "main_memory.h"
#include "clint.h"
#include "trap.h"

#ifdef UART_EN
#include "uart.h"
#define MEM_MAP_SIZE 3
#else
#define MEM_MAP_SIZE 2
#endif

struct mem_entry {
    uint32_t base;
    uint32_t size;
    dev* ptr;
};

class memory {
    private:
        main_memory mm;
        #ifdef UART_EN
        uart uart0;
        #endif
        clint clint0;
        dev *dev_ptr;
        std::array<mem_entry, MEM_MAP_SIZE> mem_map;
        trap *tu;

    private:
        uint32_t set_addr(uint32_t address, mem_op_t access, uint32_t size);

    public:
        memory() = delete;
        memory(std::string test_elf, cfg_t cfg, hw_cfg_t hw_cfg);
        std::map<uint32_t, symbol_map_entry_t> get_symbol_map() {
            return mm.get_symbol_map();
        }
        void trap_setup(trap* tu) {
            this->tu = tu;
            clint0.trap_setup(tu);
        }
        uint64_t get_mtime_shadow() { return clint0.get_mtime_shadow(); }
        void set_mip(uint32_t* csr_mip) { clint0.set_mip(csr_mip); }
        void update_mtime() { clint0.update_mtime(); }
        uint32_t rd_inst(uint32_t address);
        uint32_t just_inst(uint32_t address);
        uint32_t rd(uint32_t address, uint32_t size);
        void wr(uint32_t address, uint32_t data, uint32_t size);
        void dump_as_bytes(uint32_t start, uint32_t size);
        void dump_as_words(uint32_t start, uint32_t size, std::string out_dir);
        scp_status_t cache_hint(uint32_t address, scp_mode_t scp_mode);
        #ifdef HW_MODELS_EN
        // propagating to caches
        void log_cache_stats(std::ofstream& hw_ofs) {
            mm.log_cache_stats(hw_ofs);
        }
        void set_cache_hws(hw_status_t* ic, hw_status_t* dc) {
            mm.set_cache_hws(ic, dc);
        }
        void cache_finish(bool silent) {
            if (silent) return;
            mm.finish();
        }
        void cache_profiling(bool enable) {
            mm.cache_profiling(enable);
        }
        void speculative_exec(speculative_t smode) {
            mm.speculative_exec(smode);
        }
        #ifdef PROFILERS_EN
        void set_perf_profiler(profiler_perf* prof_perf) {
            mm.set_perf_profiler(prof_perf);
        }
        #endif
        #ifdef DASM_EN
        void set_hwmi(hwmi_str* h) {
            mm.set_hwmi(h);
        }
        #endif
        #endif
};

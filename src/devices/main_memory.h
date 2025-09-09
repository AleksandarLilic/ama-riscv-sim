#pragma once

#include "defines.h"
#include "dev.h"
#include "external/ELFIO/elfio/elfio.hpp"
#ifdef HW_MODELS_EN
#include "cache.h"
#endif

class main_memory : public dev {
    private:
        void burn_bin(std::string test_bin);
        void burn_elf(std::string test_elf);
        std::map<uint32_t, symbol_map_entry_t> symbol_map;
        #ifdef HW_MODELS_EN
        cache icache;
        cache dcache;
        const bool show_state;
        #endif

    public:
        main_memory() = delete;
        main_memory(size_t size, std::string test_elf, hw_cfg_t hw_cfg);
        std::map<uint32_t, symbol_map_entry_t> get_symbol_map() {
            return symbol_map;
        }
        uint32_t rd_inst(uint32_t addr);
        uint32_t just_inst(uint32_t addr) { return dev::rd(addr, 4); }
        virtual uint32_t rd(uint32_t addr, uint32_t size) override;
        virtual void wr(uint32_t addr, uint32_t data, uint32_t size) override;
        std::array<uint8_t, CACHE_LINE_SIZE> rd_line(uint32_t addr);
        void wr_line(uint32_t addr, std::array<uint8_t, CACHE_LINE_SIZE> data);
        #ifdef HW_MODELS_EN
        scp_status_t scp(uint32_t addr, scp_mode_t scp_mode);
        void cache_profiling(bool enable) {
            icache.profiling(enable);
            dcache.profiling(enable);
        }
        void speculative_exec(speculative_t smode) {
            icache.speculative_exec(smode);
            dcache.speculative_exec(smode);
        }
        void log_cache_stats(std::ofstream& hw_ofs) {
           icache.log_stats(hw_ofs);
           dcache.log_stats(hw_ofs);
        }
        void set_cache_hws(hw_status_t* ic, hw_status_t* dc) {
            icache.set_hws(ic);
            dcache.set_hws(dc);
        }
        void finish() {
            icache.show_stats(show_state);
            dcache.show_stats(show_state);
        }
        #ifdef PROFILERS_EN
        void set_perf_profiler(profiler_perf* prof_perf) {
            icache.set_perf_profiler(
                prof_perf,
                perf_event_t::icache_reference,
                perf_event_t::icache_miss);
            dcache.set_perf_profiler(
                prof_perf,
                perf_event_t::dcache_reference,
                perf_event_t::dcache_miss);
        }
        #endif
        #ifdef DASM_EN
        void set_hwmi(hwmi_str* h) {
            icache.set_hwmi(h);
            dcache.set_hwmi(h);
        }

        #endif
        #endif
};

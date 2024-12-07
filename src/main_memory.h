#pragma once

#include "defines.h"
#include "dev.h"
#ifdef ENABLE_HW_PROF
#include "cache.h"
#endif

class main_memory : public dev {
    private:
        void burn(std::string test_bin);
        #ifdef ENABLE_HW_PROF
        cache icache;
        cache dcache;
        #endif

    public:
        main_memory() = delete;
        main_memory(size_t size, std::string test_bin, hw_cfg_t hw_cfg);
        uint32_t rd_inst(uint32_t addr);
        virtual uint32_t rd(uint32_t addr, uint32_t size) override;
        virtual void wr(uint32_t addr, uint32_t data, uint32_t size) override;
        std::array<uint8_t, CACHE_LINE_SIZE> rd_line(uint32_t addr);
        void wr_line(uint32_t addr, std::array<uint8_t, CACHE_LINE_SIZE> data);
        #ifdef ENABLE_HW_PROF
        scp_status_t scp(uint32_t addr, scp_mode_t scp_mode);
        void cache_profiling(bool enable) {
            icache.profiling(enable);
            dcache.profiling(enable);
        }
        void speculative_exec(speculative_t smode) {
            icache.speculative_exec(smode);
            dcache.speculative_exec(smode);
        }
        void log_cache_stats(std::ofstream& log_file) {
           icache.log_stats(log_file);
           dcache.log_stats(log_file);
        }
        void finish() { icache.show_stats(); dcache.show_stats(); }
        #endif
};

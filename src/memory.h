#pragma once

#include "defines.h"
#include "main_memory.h"

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

    private:
        void mem_dump(uint32_t start, uint32_t end);
        uint32_t set_addr(uint32_t address);

    public:
        memory() = delete;
        memory(uint32_t base_address, std::string test_bin, hw_cfg_t hw_cfg);
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
        #endif
};

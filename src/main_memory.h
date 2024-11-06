#pragma once

#include "defines.h"
#include "dev.h"
#include "cache.h"

class main_memory : public dev {
    private:
        void burn(std::string test_bin);
        #ifdef ENABLE_HW_PROF
        cache icache;
        cache dcache;
        #endif

    public:
        main_memory() = delete;
        main_memory(size_t size, std::string test_bin);
        #if defined(ENABLE_HW_PROF) && !defined(DPI)
        ~main_memory() { icache.show_stats(); dcache.show_stats(); }
        #endif
        uint32_t rd_inst(uint32_t address);
        virtual uint8_t rd8(uint32_t address) override;
        virtual uint16_t rd16(uint32_t address) override;
        virtual uint32_t rd32(uint32_t address) override;
        virtual void wr8(uint32_t address, uint32_t data) override;
        virtual void wr16(uint32_t address, uint32_t data) override;
        virtual void wr32(uint32_t address, uint32_t data) override;
        //std::array<uint8_t, CACHE_LINE_SIZE> rd_line(uint32_t address);
        #ifdef ENABLE_HW_PROF
        void log_cache_stats(std::ofstream& log_file) {
           icache.log_stats(log_file);
           dcache.log_stats(log_file);
        }
        #endif
};

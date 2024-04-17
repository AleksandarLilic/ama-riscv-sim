#pragma once

#include "defines.h"
#include "main_memory.h"
#ifdef UART_ENABLE
#include "uart.h"
#endif

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
        memory(uint32_t base_address, std::string test_bin);
        uint8_t rd8(uint32_t address);
        uint16_t rd16(uint32_t address);
        uint32_t rd32(uint32_t address);
        uint32_t get_inst(uint32_t address);
        void wr8(uint32_t address, uint32_t data);
        void wr16(uint32_t address, uint32_t data);
        void wr32(uint32_t address, uint32_t data);
        void dump();
        void dump(uint32_t start, uint32_t size);
};

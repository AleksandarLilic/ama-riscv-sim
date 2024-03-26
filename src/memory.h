#pragma once

#include "defines.h"

class memory {
    private:
        std::array<uint8_t, MEM_SIZE> mem;
        uint32_t base_address;
    private:
        void burn(std::string test_bin);
        void mem_dump(uint32_t start, uint32_t end);
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
        void dump(uint32_t start, uint32_t end);
};

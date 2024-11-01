#pragma once

#include "defines.h"

class dev {
    protected:
        std::vector<uint8_t> mem;

    public:
        dev() = delete;
        dev(size_t size) : mem(size) { std::fill(mem.begin(), mem.end(), 0xA5);}

        virtual uint8_t rd8(uint32_t address);
        virtual uint16_t rd16(uint32_t address);
        virtual uint32_t rd32(uint32_t address);
        virtual void wr8(uint32_t address, uint32_t data);
        virtual void wr16(uint32_t address, uint32_t data);
        virtual void wr32(uint32_t address, uint32_t data);
};

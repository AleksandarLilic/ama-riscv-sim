#pragma once

#include "defines.h"

class dev {
    protected:
        std::vector<uint8_t> mem;

    public:
        dev() = delete;
        dev(size_t size) : mem(size, 0xA5) { };
        virtual ~dev() = default;
        virtual uint32_t rd(uint32_t addr, uint32_t size);
        virtual void wr(uint32_t addr, uint32_t data, uint32_t size);
        virtual uint8_t rd_8(uint32_t addr);
        virtual uint16_t rd_16(uint32_t addr);
        virtual uint32_t rd_32(uint32_t addr);
        virtual uint64_t rd_64(uint32_t addr);
        virtual void wr_8(uint32_t addr, uint8_t data);
        virtual void wr_16(uint32_t addr, uint16_t data);
        virtual void wr_32(uint32_t addr, uint32_t data);
        virtual void wr_64(uint32_t addr, uint64_t data);
};

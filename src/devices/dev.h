#pragma once

#include "defines.h"

class dev {
    protected:
        std::vector<uint8_t> mem;

    public:
        dev() = delete;
        dev(size_t size) : mem(size, 0xA5) { };
        virtual uint32_t rd(uint32_t address, uint32_t size);
        virtual void wr(uint32_t address, uint32_t data, uint32_t size);
        virtual ~dev() = default;
};

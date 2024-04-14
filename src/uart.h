#pragma once

#include "defines.h"
#include "dev.h"

class uart : public dev {
    public:
        uart(size_t size);
        void wr(uint32_t address, uint8_t data);
};
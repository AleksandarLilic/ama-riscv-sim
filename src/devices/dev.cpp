#include "dev.h"

uint32_t dev::rd(uint32_t address, uint32_t size) {
    uint32_t data = 0;
    for (uint32_t i = 0; i < size; i++)
        data |= (mem[address + i] << (i * 8));
    return data;
}

void dev::wr(uint32_t address, uint32_t data, uint32_t size) {
    for (uint32_t i = 0; i < size; i++)
        mem[address + i] = TO_U8(data >> (i * 8));
}

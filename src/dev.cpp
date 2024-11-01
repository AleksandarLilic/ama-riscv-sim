#include "dev.h"

uint8_t dev::rd8(uint32_t address) {
    return mem[address];
}

uint16_t dev::rd16(uint32_t address) {
    return TO_U16(mem[address]) |
           (TO_U16(mem[address + 1]) << 8);
}

uint32_t dev::rd32(uint32_t address) {
    return TO_U32(mem[address]) |
           (TO_U32(mem[address + 1]) << 8) |
           (TO_U32(mem[address + 2]) << 16) |
           (TO_U32(mem[address + 3]) << 24);
}

void dev::wr8(uint32_t address, uint32_t data) {
    mem[address] = TO_U8(data);
}

void dev::wr16(uint32_t address, uint32_t data) {
    mem[address] = TO_U8(data & 0xFF);
    mem[address + 1] = TO_U8((data >> 8) & 0xFF);
}

void dev::wr32(uint32_t address, uint32_t data) {
    mem[address] = TO_U8(data & 0xFF);
    mem[address + 1] = TO_U8((data >> 8) & 0xFF);
    mem[address + 2] = TO_U8((data >> 16) & 0xFF);
    mem[address + 3] = TO_U8((data >> 24) & 0xFF);
}

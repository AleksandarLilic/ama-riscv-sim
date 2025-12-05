#include "dev.h"

inline uint32_t dev::rd(uint32_t addr, uint32_t size) {
    #if defined(DEBUG)
    if (__builtin_expect((size != 1) && (size != 2) && (size != 4), 0)){
        __builtin_trap();
    }
    #endif

    switch (size) {
        case 1: return rd_8(addr);
        case 2: return rd_16(addr);
        case 4: return rd_32(addr);
        default: __builtin_unreachable();
    }
}

inline void dev::wr(uint32_t addr, uint32_t data, uint32_t size) {
    #if defined(DEBUG)
    if (__builtin_expect((size != 1) && (size != 2) && (size != 4), 0)){
        __builtin_trap();
    }
    #endif

    switch (size) {
        case 1: wr_8(addr, TO_U8(data)); break;
        case 2: wr_16(addr, TO_U16(data)); break;
        case 4: wr_32(addr, TO_U32(data)); break;
        default: __builtin_unreachable();
    }
}

// per width reads
inline uint8_t dev::rd_8(uint32_t addr) {
    return mem[addr];
}

inline uint16_t dev::rd_16(uint32_t addr) {
    return (
        TO_U16(mem[addr]) |
        (TO_U16(mem[addr + 1]) << 8)
    );
}

inline uint32_t dev::rd_32(uint32_t addr) {
    return (
        TO_U32(mem[addr]) |
        (TO_U32(mem[addr + 1]) << 8) |
        (TO_U32(mem[addr + 2]) << 16) |
        (TO_U32(mem[addr + 3]) << 24)
    );
}

inline uint64_t dev::rd_64(uint32_t addr) {
    return (
        TO_U64(mem[addr]) |
        (TO_U64(mem[addr + 1]) << 8) |
        (TO_U64(mem[addr + 2]) << 16) |
        (TO_U64(mem[addr + 3]) << 24) |
        (TO_U64(mem[addr + 4]) << 32) |
        (TO_U64(mem[addr + 5]) << 40) |
        (TO_U64(mem[addr + 6]) << 48) |
        (TO_U64(mem[addr + 7]) << 56)
    );
}

// per width writes
inline void dev::wr_8(uint32_t addr, uint8_t data) {
    mem[addr] = data;
}

inline void dev::wr_16(uint32_t addr, uint16_t data) {
    mem[addr] = TO_U8(data);
    mem[addr + 1] = TO_U8(data >> 8);
}

inline void dev::wr_32(uint32_t addr, uint32_t data) {
    mem[addr] = TO_U8(data);
    mem[addr + 1] = TO_U8(data >> 8);
    mem[addr + 2] = TO_U8(data >> 16);
    mem[addr + 3] = TO_U8(data >> 24);
}

inline void dev::wr_64(uint32_t addr, uint64_t data) {
    mem[addr] = TO_U8(data);
    mem[addr + 1] = TO_U8(data >> 8);
    mem[addr + 2] = TO_U8(data >> 16);
    mem[addr + 3] = TO_U8(data >> 24);
    mem[addr + 4] = TO_U8(data >> 32);
    mem[addr + 5] = TO_U8(data >> 40);
    mem[addr + 6] = TO_U8(data >> 48);
    mem[addr + 7] = TO_U8(data >> 56);
}

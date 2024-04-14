#include "uart.h"

uart::uart(size_t size) : dev(size) { }

inline void uart::wr(uint32_t address, uint8_t data) {
    mem[address] = data;
    std::cout << data;
}

// TODO: implement read from uart/serial
// TODO: implement registers to communicate with CPU (e.g. status, control, etc.)

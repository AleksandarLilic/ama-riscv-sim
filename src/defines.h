#pragma once

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <array>
#include <fstream>
#include <string>
#include <cmath>

// Macros
#define CHECK_ADDRESS(address, align) \
    if ((address % 4u) + align > 4u) { \
        std::cerr << "ERROR: Unaligned access at address: 0x" \
                  << std::hex << address \
                  << std::dec << "; for: " << align << " bytes" << std::endl; \
    } \
    address -= base_address; \
    if(address > MEM_SIZE) { \
        std::cerr << "ERROR: Address out of range: " \
                  << std::hex << address << std::endl; \
}

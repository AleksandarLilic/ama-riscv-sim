#pragma once

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <array>
#include <fstream>
#include <string>
#include <cmath>
#include <unordered_map>

#define MEM_SIZE 1024
const uint32_t MEM_ADDR_BITWIDTH = std::log10(MEM_SIZE) + 1;

// Decoder types
enum class opcode{ 
    al_reg = 0b011'0011, // R format
    al_imm = 0b001'0011, // I format
    load = 0b000'0011, // I format
    store = 0b010'0011, // S format
    branch = 0b110'0011, // B format
    jalr = 0b110'0111, // I format
    jal = 0b110'1111, // J format
    lui = 0b011'0111, // U format
    auipc = 0b001'0111, // U format
    system = 0b111'0011 // I format
};

enum class alu_op_t {
    op_add = (0b0000),
    op_sub = (0b1000),
    op_sll = (0b0001),
    op_srl = (0b0101),
    op_sra = (0b1101),
    op_slt = (0b0010),
    op_sltu = (0b0011),
    op_xor = (0b0100),
    op_or = (0b0110),
    op_and = (0b0111)
};

enum class load_op_t {
    op_byte = 0b000,
    op_half = 0b001,
    op_word = 0b010,
    op_byte_u = 0b100,
    op_half_u = 0b101,
};

enum class store_op_t {
    op_byte = 0b000,
    op_half = 0b001,
    op_word = 0b010
};

enum class branch_op_t {
    op_beq = 0b000,
    op_bne = 0b001,
    op_blt = 0b100,
    op_bge = 0b101,
    op_bltu = 0b110,
    op_bgeu = 0b111
};

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

#define MEM_ADDR_FORMAT(addr) \
    std::setw(MEM_ADDR_BITWIDTH) << std::setfill('0') << std::hex << addr

#define FRF_DEF(x,y) \
    std::left << std::setw(2) << std::setfill(' ') \
              << x << ": " << std::left << std::setw(12) << int32_t(y)
#define FRF(x,y) "  x" << FRF_DEF(x,y) << "  "

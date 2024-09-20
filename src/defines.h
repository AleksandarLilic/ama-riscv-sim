#pragma once

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <array>
#include <vector>
#include <fstream>
#include <string>
#include <cmath>
#include <unordered_map>

#include "dev.h"

#define TO_U32(x) static_cast<uint32_t>(x)
#define TO_I32(x) static_cast<int32_t>(x)
#define TO_U16(x) static_cast<uint16_t>(x)
#define TO_I16(x) static_cast<int16_t>(x)
#define TO_U8(x) static_cast<uint8_t>(x)
#define TO_I8(x) static_cast<int8_t>(x)

#ifdef USE_ABI_NAMES
#define RF_NAMES 1u
#define FRF_W 4
#else
#define RF_NAMES 0u
#define FRF_W 3
#endif

#define BASE_ADDR 0x80000000
#define MEM_ADDR_BITWIDTH 8
#define MEM_SIZE 16384 // 0x4000
//#define MEM_SIZE 131072 // 0x20000
//#define MEM_SIZE 196608 // 0x30000
//#define MEM_SIZE 262144 // 0x40000
//#define MEM_SIZE 524288 // 0x80000
#define UART0_SIZE 12 // 3 32-bit registers

// Decoder types
enum class opcode {
    al_reg = 0b011'0011, // R type
    al_imm = 0b001'0011, // I type
    load = 0b000'0011, // I type
    store = 0b010'0011, // S type
    branch = 0b110'0011, // B type
    jalr = 0b110'0111, // I type
    jal = 0b110'1111, // J type
    lui = 0b011'0111, // U type
    auipc = 0b001'0111, // U type
    system = 0b111'0011, // I type
    misc_mem = 0b000'1111 // I type
};

enum class alu_r_op_t {
    op_add = 0b0000,
    op_sub = 0b1000,
    op_sll = 0b0001,
    op_srl = 0b0101,
    op_sra = 0b1101,
    op_slt = 0b0010,
    op_sltu = 0b0011,
    op_xor = 0b0100,
    op_or = 0b0110,
    op_and = 0b0111
};

enum class alu_r_mul_op_t {
    op_mul = 0b0000,
    op_mulh = 0b0001,
    op_mulhsu = 0b0010,
    op_mulhu = 0b0011,
    op_div = 0b0100,
    op_divu = 0b0101,
    op_rem = 0b0110,
    op_remu = 0b0111
};

enum class alu_i_op_t {
    op_addi = 0b0000,
    op_slli = 0b0001,
    op_srli = 0b0101,
    op_srai = 0b1101,
    op_slti = 0b0010,
    op_sltiu = 0b0011,
    op_xori = 0b0100,
    op_ori = 0b0110,
    op_andi = 0b0111
};

enum class load_op_t {
    op_lb = 0b000,
    op_lh = 0b001,
    op_lw = 0b010,
    op_lbu = 0b100,
    op_lhu = 0b101,
};

enum class store_op_t {
    op_sb = 0b000,
    op_sh = 0b001,
    op_sw = 0b010
};

enum class branch_op_t {
    op_beq = 0b000,
    op_bne = 0b001,
    op_blt = 0b100,
    op_bge = 0b101,
    op_bltu = 0b110,
    op_bgeu = 0b111
};

enum class csr_op_t {
    op_rw = 0b001,
    op_rs = 0b010,
    op_rc = 0b011,
    op_rwi = 0b101,
    op_rsi = 0b110,
    op_rci = 0b111
};

struct dasm_str {
    std::ostringstream asm_ss;
    std::string asm_str;
    std::string op;
};

// Instruction field masks
#define M_OPC7 uint32_t(0x7F)
#define M_OPC2 uint32_t(0x3)

#define M_FUNCT7 uint32_t((0x7F)<<25)
#define M_FUNCT7_B5 uint32_t((0x1)<<30)
#define M_FUNCT7_B1 uint32_t((0x1)<<25)
#define M_FUNCT3 uint32_t((0x7)<<12)
#define M_CFUNCT2H uint32_t((0x3)<<10)
#define M_CFUNCT2L uint32_t((0x3)<<5)
#define M_CFUNCT3 uint32_t((0x7)<<13)
#define M_CFUNCT4 uint32_t((0xF)<<12)
#define M_CFUNCT6 uint32_t((0x3F)<<10)

#define M_RD uint32_t((0x1F)<<7)
#define M_RS1 uint32_t((0x1F)<<15)
#define M_RS2 uint32_t((0x1F)<<20)
#define M_CRS2 uint32_t((0x1F)<<2)
#define M_CREGH uint32_t((0x7)<<7)
#define M_CREGL uint32_t((0x7)<<2)
#define M_IMM_SHAMT uint32_t(0x1F)

#define M_IMM_31_25 int32_t((0x7F)<<25)
#define M_IMM_31_20 int32_t((0xFFF)<<20)
#define M_IMM_30_25 uint32_t((0x3F)<<25)
#define M_IMM_24_21 uint32_t((0xF)<<21)
#define M_IMM_24_20 uint32_t((0x1F)<<20)
#define M_IMM_19_12 uint32_t((0xFF)<<12)
#define M_IMM_12_11 uint32_t((0x3)<<11)
#define M_IMM_12_10 uint32_t((0x7)<<10)
#define M_IMM_12_9 uint32_t((0xF)<<9)
#define M_IMM_11_10 uint32_t((0x3)<<10)
#define M_IMM_11_8 uint32_t((0xF)<<8)
#define M_IMM_10_9 uint32_t((0x3)<<9)
#define M_IMM_10_7 uint32_t((0xF)<<7)
#define M_IMM_8_7 uint32_t((0x3)<<7)
#define M_IMM_6_5 uint32_t((0x3)<<5)
#define M_IMM_6_4 uint32_t((0x7)<<4)
#define M_IMM_6_2 uint32_t((0x1F)<<2)
#define M_IMM_5_3 uint32_t((0x7)<<3)
#define M_IMM_4_3 uint32_t((0x3)<<3)
#define M_IMM_3_2 uint32_t((0x3)<<2)

#define M_IMM_31 int32_t((0x1)<<31)
#define M_IMM_20 uint32_t((0x1)<<20)
#define M_IMM_12 uint32_t((0x1)<<12)
#define M_IMM_11 uint32_t((0x1)<<11)
#define M_IMM_8 uint32_t((0x1)<<8)
#define M_IMM_7 uint32_t((0x1)<<7)
#define M_IMM_6 uint32_t((0x1)<<6)
#define M_IMM_5 uint32_t((0x1)<<5)
#define M_IMM_2 uint32_t((0x1)<<2)

// Instructions
#define INST_ECALL 0x73
#define INST_EBREAK 0x100073
#define INST_FENCE_I 0x100F
#define INST_NOP 0x13
#define INST_C_NOP 0x1

// CSRs
struct CSR {
    const char* name;
    int value;
    CSR() : name(""), value(0) {} // FIXME
    CSR(const char* name, int value) : name(name), value(value) {}
};

struct CSR_entry {
    const uint16_t csr_addr;
    const char* csr_name;
};

// Macros
#define CASE_DECODER(op) \
    case (uint8_t)opcode::op: \
        op(); \
        break;

#define CASE_ALU_REG_OP(op) \
    case (uint8_t)alu_r_op_t::op_##op: \
        write_rf(get_rd(), al_##op(rf[get_rs1()], rf[get_rs2()])); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        break;

#define CASE_ALU_REG_MUL_OP(op) \
    case (uint8_t)alu_r_mul_op_t::op_##op: \
        write_rf(get_rd(), al_##op(rf[get_rs1()], rf[get_rs2()])); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        break;

#define CASE_ALU_IMM_OP(op) \
    case (uint8_t)alu_i_op_t::op_##op: \
        write_rf(get_rd(), al_##op(rf[get_rs1()], get_imm_i())); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1 \
        break;

#define CASE_LOAD(op) \
    case (uint8_t)load_op_t::op_##op: \
        write_rf(get_rd(), load_##op((rf[get_rs1()]+get_imm_i()))); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1 \
        break;

#define CASE_STORE(op) \
    case (uint8_t)store_op_t::op_##op: \
        store_##op(rf[get_rs1()]+get_imm_s(), rf[get_rs2()]); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RS1_RS2 \
        break;

#define CASE_BRANCH(op) \
    case (uint8_t)branch_op_t::op_##op: \
        if(branch_##op()) { \
            next_pc = pc + get_imm_b(); \
            PROF_B_T(op) \
        } else { \
            PROF_B_NT(op, _b) \
        } \
        DASM_OP(op) \
        PROF_RS1_RS2 \
        break;

#define CASE_CSR(op, val) \
    case (uint8_t)csr_op_t::op_##op: \
        csr_##op(val); \
        DASM_OP(csr##op) \
        PROF_G(csr##op) \
        PROF_RD_RS1 \
        DASM_CSR_REG \
        break;

#define CASE_CSR_I(op) \
    case (uint8_t)csr_op_t::op_##op: \
        csr_##op(); \
        DASM_OP(csr##op) \
        PROF_G(csr##op) \
        PROF_RD \
        DASM_CSR_IMM \
        break;

#define W_CSR(expr) \
    write_csr(get_csr_addr(), expr)

struct mem_entry {
    uint32_t base;
    uint32_t size;
    dev *ptr;
};

#define MEM_OUT_OF_RANGE(addr, reson) \
    do { \
        std::stringstream ss; \
        ss << MEM_ADDR_FORMAT(address); \
        std::string hex_addr = ss.str(); \
        throw std::runtime_error("Address " + hex_addr + \
                                " out of range: " + reson); \
    } while(0);

//#define CHECK_ADDRESS(address, align)

#define CHECK_ADDRESS(address, align) \
    do { \
        bool address_out_of_range = (address >= MEM_SIZE + UART0_SIZE); \
        bool address_unaligned = ((address % 4u) + align > 4u); \
        if (address_out_of_range || address_unaligned) { \
            if (address_out_of_range) { \
                std::cerr << "ERROR: Address out of range: 0x" \
                          << std::hex << address << std::dec << std::endl; \
            } \
            else { \
                std::cerr << "ERROR: Unaligned access at address: 0x" \
                          << std::hex << address \
                          << std::dec << "; for: " << align << " bytes" \
                          << std::endl; \
            } \
        } \
    } while(0);

#define MEM_ADDR_FORMAT(addr) \
    std::setw(MEM_ADDR_BITWIDTH) << std::setfill('0') << std::hex << addr

#define INST_FORMAT(inst, n) \
    std::setw(n) << std::setfill('0') << std::hex << inst << std::dec

#define FORMAT_INST(inst, n) \
    MEM_ADDR_FORMAT(pc) << ": " << INST_FORMAT(inst, n)

// Format Register File print
#define FRF(addr, val) \
    std::left << std::setw(FRF_W) << std::setfill(' ') << addr \
              << ": 0x" << std::right << std::setw(8) << std::setfill('0') \
              << std::hex << val << std::dec << "  "

// Format CSR print
#define CSRF(it) \
    std::hex << "0x" << std::right << std::setw(4) << std::setfill('0') \
             << it->first << " (" << it->second.name << ")" \
             << ": 0x" << std::left << std::setw(8) << std::setfill(' ') \
             << it->second.value << std::dec

#ifdef ENABLE_DASM
#define DASM_OP(o) \
    dasm.op = #o;

#define DASM_CSR_REG \
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << "," \
                << csr.at(get_csr_addr()).name << "," \
                << rf_names[get_rs1()][RF_NAMES];

#define DASM_CSR_IMM \
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << "," \
                << csr.at(get_csr_addr()).name << "," \
                << get_uimm_csr();

#define DASM_OP_RD \
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES]

#define DASM_OP_CREGH \
    dasm.asm_ss << dasm.op << " " << rf_names[get_cregh()][RF_NAMES]

#define DASM_CREGL \
    rf_names[get_cregl()][RF_NAMES]

#else
#define DASM_OP(o)
#define DASM_CSR_REG
#define DASM_CSR_IMM
#define DASM_OP_RD
#define DASM_OP_CREGH
#endif

#if defined(ENABLE_DASM) || defined(ENABLE_PROF)
#define INST_W(x) \
    inst_w = x;
#else
#define INST_W(x)
#endif

#ifdef ENABLE_PROF
#define PROF_G(op) \
    prof.log_inst(opc_g::i_##op);

#define PROF_J(op) \
    prof.log_inst(opc_j::i_##op, true, b_dir_t(next_pc > pc));

#define PROF_B_T(op) \
    prof.log_inst(opc_j::i_##op, true, b_dir_t(next_pc > pc));

#define PROF_B_NT(op, b) \
    prof.log_inst(opc_j::i_##op, false, b_dir_t((pc + get_imm##b()) > pc));

#define PROF_RD \
    prof.log_reg_use(reg_use_t::rd, get_rd()); \

#define PROF_RS1 \
    prof.log_reg_use(reg_use_t::rs1, get_rs1());

#define PROF_RS2 \
    prof.log_reg_use(reg_use_t::rs2, get_rs2());

#define PROF_RD_RS1_RS2 \
    PROF_RD \
    PROF_RS1 \
    PROF_RS2

#define PROF_RD_RS1 \
    PROF_RD \
    PROF_RS1

#define PROF_RS1_RS2 \
    PROF_RS1 \
    PROF_RS2

#define PROF_DMEM(size) \
    prof.te.dmem_size = size; \
    prof.te.dmem = addr - BASE_ADDR;

#else
#define PROF_G(op)
#define PROF_J(op)
#define PROF_B_T(op)
#define PROF_B_NT(op, b)
#define PROF_RD
#define PROF_RS1
#define PROF_RS2
#define PROF_RD_RS1_RS2
#define PROF_RD_RS1
#define PROF_RS1_RS2
#define PROF_DMEM(addr)
#endif

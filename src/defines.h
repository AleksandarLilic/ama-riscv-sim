#pragma once

#include "types.h"

// default defines
#ifndef DPI
#define UART_ENABLE
#define ENABLE_PROF
#define ENABLE_HW_PROF
#endif

// casts
#define TO_F64(x) static_cast<double_t>(x)
#define TO_F32(x) static_cast<float_t>(x)
#define TO_U64(x) static_cast<uint64_t>(x)
#define TO_I64(x) static_cast<int64_t>(x)
#define TO_U32(x) static_cast<uint32_t>(x)
#define TO_I32(x) static_cast<int32_t>(x)
#define TO_U16(x) static_cast<uint16_t>(x)
#define TO_I16(x) static_cast<int16_t>(x)
#define TO_U8(x) static_cast<uint8_t>(x)
#define TO_I8(x) static_cast<int8_t>(x)
#define TO_U4(x) static_cast<uint8_t>(x & 0xF)
#define TO_I4(x) static_cast<int8_t>((x & 0xF) | ((x & 0x8) ? 0xF0 : 0x00))
#define TO_U2(x) static_cast<uint8_t>(x & 0x3)
#define TO_I2(x) static_cast<int8_t>((x & 0x3) | ((x & 0x2) ? 0xFC : 0x00))

// Memory
#define BASE_ADDR 0x10000
#define ADDR_BITS 17 // 128KB address space
#define MEM_ADDR_BITWIDTH 5 // digits in hex printout
//#define MEM_SIZE 16384
#define MEM_SIZE 32768
//#define MEM_SIZE 65536
#define UART0_SIZE 12 // 3 32-bit registers

// HW models
#define CACHE_MODE_PERF 0 // tags and stats
#define CACHE_MODE_FUNC 1 // adds data
//#define CACHE_VERIFY // only for CACHE_MODE_FUNC

#ifndef CACHE_MODE
#define CACHE_MODE CACHE_MODE_PERF
#endif

#define CACHE_LINE_SIZE 64 // bytes
#define CACHE_BYTE_ADDR_BITS (__builtin_ctz(CACHE_LINE_SIZE)) // 6
#define CACHE_BYTE_ADDR_MASK (CACHE_LINE_SIZE - 1) // 0x3F, bottom 6 bits
//#define CACHE_ADDR_MASK 1FFFF // 17 bits

// Instruction field masks
#define M_OPC7 TO_U32(0x7F)
#define M_OPC2 TO_U32(0x3)

#define M_FUNCT7 TO_U32((0x7F)<<25)
#define M_FUNCT7_B5 TO_U32((0x1)<<30)
#define M_FUNCT7_B1 TO_U32((0x1)<<25)
#define M_FUNCT3 TO_U32((0x7)<<12)
#define M_CFUNCT2H TO_U32((0x3)<<10)
#define M_CFUNCT2L TO_U32((0x3)<<5)
#define M_CFUNCT3 TO_U32((0x7)<<13)
#define M_CFUNCT4 TO_U32((0xF)<<12)
#define M_CFUNCT6 TO_U32((0x3F)<<10)

#define M_RD TO_U32((0x1F)<<7)
#define M_RS1 TO_U32((0x1F)<<15)
#define M_RS2 TO_U32((0x1F)<<20)
#define M_CRS2 TO_U32((0x1F)<<2)
#define M_CREGH TO_U32((0x7)<<7)
#define M_CREGL TO_U32((0x7)<<2)
#define M_IMM_SHAMT TO_U32(0x1F)

#define M_IMM_31_25 TO_I32((0x7F)<<25)
#define M_IMM_31_20 TO_I32((0xFFF)<<20)
#define M_IMM_30_25 TO_U32((0x3F)<<25)
#define M_IMM_24_21 TO_U32((0xF)<<21)
#define M_IMM_24_20 TO_U32((0x1F)<<20)
#define M_IMM_19_12 TO_U32((0xFF)<<12)
#define M_IMM_12_11 TO_U32((0x3)<<11)
#define M_IMM_12_10 TO_U32((0x7)<<10)
#define M_IMM_12_9 TO_U32((0xF)<<9)
#define M_IMM_11_10 TO_U32((0x3)<<10)
#define M_IMM_11_8 TO_U32((0xF)<<8)
#define M_IMM_10_9 TO_U32((0x3)<<9)
#define M_IMM_10_7 TO_U32((0xF)<<7)
#define M_IMM_8_7 TO_U32((0x3)<<7)
#define M_IMM_6_5 TO_U32((0x3)<<5)
#define M_IMM_6_4 TO_U32((0x7)<<4)
#define M_IMM_6_2 TO_U32((0x1F)<<2)
#define M_IMM_5_3 TO_U32((0x7)<<3)
#define M_IMM_4_3 TO_U32((0x3)<<3)
#define M_IMM_3_2 TO_U32((0x3)<<2)

#define M_IMM_31 TO_I32((0x1)<<31)
#define M_IMM_20 TO_U32((0x1)<<20)
#define M_IMM_12 TO_U32((0x1)<<12)
#define M_IMM_11 TO_U32((0x1)<<11)
#define M_IMM_8 TO_U32((0x1)<<8)
#define M_IMM_7 TO_U32((0x1)<<7)
#define M_IMM_6 TO_U32((0x1)<<6)
#define M_IMM_5 TO_U32((0x1)<<5)
#define M_IMM_2 TO_U32((0x1)<<2)

// Instructions
#define INST_ECALL 0x73
#define INST_EBREAK 0x100073
#define INST_FENCE_I 0x100F
#define INST_NOP 0x13
#define INST_C_NOP 0x1
#define INST_HINT_LOG_START 0x01002013 // slti x0, x0, 0x10
#define INST_HINT_LOG_END 0x01102013 // slti x0, x0, 0x11

// CSR addresses
#define CSR_TOHOST 0x51e
#define CSR_MSCRATCH 0x340
#define CSR_MCYCLE 0xb00
#define CSR_MINSTRET 0xb02
#define CSR_MCYCLEH 0xb80
#define CSR_MINSTRETH 0xb82
// read-only CSRs
#define CSR_MISA 0x301
#define CSR_MHARTID 0xf14
// read only user CSRs
#define CSR_CYCLE 0xC00
#define CSR_TIME 0xC01
#define CSR_INSTRET 0xC02
#define CSR_CYCLEH 0xC80
#define CSR_TIMEH 0xC81
#define CSR_INSTRETH 0xC82

// Macros
#define CASE_DECODER(op) \
    case TO_U8(opcode::op): \
        op(); \
        break;

#define CASE_ALU_REG_OP(op) \
    case TO_U8(alu_r_op_t::op_##op): \
        write_rf(ip.rd(), al_##op(rf[ip.rs1()], rf[ip.rs2()])); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        break;

#define CASE_ALU_REG_MUL_OP(op) \
    case TO_U8(alu_r_mul_op_t::op_##op): \
        write_rf(ip.rd(), al_##op(rf[ip.rs1()], rf[ip.rs2()])); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        break;

#define CASE_ALU_IMM_OP(op) \
    case TO_U8(alu_i_op_t::op_##op): \
        write_rf(ip.rd(), al_##op(rf[ip.rs1()], ip.imm_i())); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1 \
        break;

#define CASE_LOAD(op) \
    case TO_U8(load_op_t::op_##op): \
        write_rf(ip.rd(), load_##op((rf[ip.rs1()]+ip.imm_i()))); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1 \
        break;

#define CASE_STORE(op) \
    case TO_U8(store_op_t::op_##op): \
        store_##op(rf[ip.rs1()]+ip.imm_s(), rf[ip.rs2()]); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RS1_RS2 \
        break;

#define CASE_BRANCH(op) \
    case TO_U8(branch_op_t::op_##op): \
        if(branch_##op()) { \
            next_pc = pc + ip.imm_b(); \
            PROF_B_T(op) \
        } else { \
            PROF_B_NT(op, _b) \
        } \
        DASM_OP(op) \
        PROF_RS1_RS2 \
        break;

#define CASE_ALU_CUSTOM_OP(op) \
    case TO_U8(alu_custom_op_t::op_##op): \
        write_rf(ip.rd(), al_c_##op(rf[ip.rs1()], rf[ip.rs2()])); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        break;

#define CASE_MEM_CUSTOM_OP(op) \
    case TO_U8(mem_custom_op_t::op_##op): \
        write_rf_pair(ip.rd(), mem_c_##op(rs1)); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RDP_RS1 \
        break;

#define CASE_SCP_CUSTOM(op) \
    case TO_U8(scp_custom_op_t::op_##op): \
        write_rf(ip.rd(), \
                 TO_U32(mem->cache_hint(rf[ip.rs1()], scp_mode_t::m_##op))); \
        PROF_G(scp_##op) \
        PROF_RD_RS1 \

#define CASE_CSR(op) \
    case TO_U8(csr_op_t::op_##op): \
        csr_##op(init_val_rs1); \
        DASM_OP(csr##op) \
        PROF_G(csr##op) \
        PROF_RD_RS1 \
        DASM_CSR_REG \
        break;

#define CASE_CSR_I(op) \
    case TO_U8(csr_op_t::op_##op): \
        csr_##op(); \
        DASM_OP(csr##op) \
        PROF_G(csr##op) \
        PROF_RD \
        DASM_CSR_IMM \
        break;

#define W_CSR(expr) write_csr(ip.csr_addr(), expr)

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
                          << std::hex << address << std::dec << " (" \
                          << align << "B)" << std::endl; \
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
    std::setw(MEM_ADDR_BITWIDTH) << std::setfill('0') << std::hex << addr \
                                 << std::dec

#define INST_FORMAT(inst, n) \
    std::setw(n) << std::setfill('0') << std::hex << inst << std::dec

#define FORMAT_INST(inst, n) \
    MEM_ADDR_FORMAT(pc) << ": " << INST_FORMAT(inst, n)

#define FHEXZ(val, w) \
    "0x" << std::setw(w) << std::setfill('0') << std::hex << val << std::dec

#define FHEXN(val, w) \
    "0x" << std::left << std::setw(w) << std::setfill(' ') << std::hex \
         << val << std::dec

// Format Register File print
#define FRF(addr, val) \
    std::left << std::setw(rf_names_w) << std::setfill(' ') << addr \
              << ": 0x" << std::right << std::setw(8) << std::setfill('0') \
              << std::hex << val << std::dec << "  "

// Format CSR print
#define CSRF(it) \
    std::hex << "0x" << std::right << std::setw(4) << std::setfill('0') \
             << it->first << " (" << it->second.name << ")" \
             << ": 0x" << std::left << std::setw(8) << std::setfill(' ') \
             << it->second.value << std::dec

#ifdef ENABLE_DASM
#define DASM_OP(o) dasm.op = #o;

#define DASM_CSR_REG \
    dasm.asm_ss << dasm.op << " " << rf_names[ip.rd()][rf_names_idx] << "," \
                << csr.at(ip.csr_addr()).name << "," \
                << rf_names[ip.rs1()][rf_names_idx];

#define DASM_CSR_IMM \
    dasm.asm_ss << dasm.op << " " << rf_names[ip.rd()][rf_names_idx] << "," \
                << csr.at(ip.csr_addr()).name << "," \
                << ip.uimm_csr();

#define DASM_OP_RD \
    dasm.asm_ss << dasm.op << " " << rf_names[ip.rd()][rf_names_idx]

#define DASM_OP_CREGH \
    dasm.asm_ss << dasm.op << " " << rf_names[ip.cregh()][rf_names_idx]

#define DASM_CREGL \
    rf_names[ip.cregl()][rf_names_idx]

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
    b_dir_t dir = (next_pc > pc) ? b_dir_t::forward : b_dir_t::backward; \
    prof.log_inst(opc_j::i_##op, true, b_dir_t(dir));

#define PROF_B_T(op) \
    b_dir_t dir = (next_pc > pc) ? b_dir_t::forward : b_dir_t::backward; \
    prof.log_inst(opc_j::i_##op, true, b_dir_t(dir));

#define PROF_B_NT(op, b) \
    b_dir_t dir = ((pc + ip.imm##b()) > pc) ? b_dir_t::forward : \
                                              b_dir_t::backward; \
    prof.log_inst(opc_j::i_##op, false, b_dir_t(dir));

#define PROF_RD \
    prof.log_reg_use(reg_use_t::rd, ip.rd()); \

#define PROF_RDP \
    prof.log_reg_use(reg_use_t::rd, ip.rd()+1); \

#define PROF_RS1 \
    prof.log_reg_use(reg_use_t::rs1, ip.rs1());

#define PROF_RS2 \
    prof.log_reg_use(reg_use_t::rs2, ip.rs2());

#define PROF_RD_RS1_RS2 \
    PROF_RD \
    PROF_RS1 \
    PROF_RS2

#define PROF_RD_RDP_RS1 \
    PROF_RD \
    PROF_RDP \
    PROF_RS1

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
#define PROF_RDP
#define PROF_RS1
#define PROF_RS2
#define PROF_RD_RS1_RS2
#define PROF_RD_RDP_RS1
#define PROF_RD_RS1
#define PROF_RS1_RS2
#define PROF_DMEM(addr)
#endif

#define INDENT "    "

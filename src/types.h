#pragma once

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <array>
#include <vector>
#include <fstream>
#include <string>
#include <cmath>
#include <map>
#include <chrono>

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
    misc_mem = 0b000'1111, // I type
    custom_ext = 0b0000'1011
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

enum class custom_ext_t {
    arith = 0b000,
    memory = 0b001,
    hints = 0b010,
};

enum class alu_custom_op_t {
    op_fma16 = 0x06,
    op_fma8 = 0x07,
    op_fma4 = 0x46,
};

enum class mem_custom_op_t {
    op_unpk16 = 0x00,
    op_unpk16u = 0x20,
    op_unpk8 = 0x01,
    op_unpk8u = 0x21,
    op_unpk4 = 0x40,
    op_unpk4u = 0x60,
    op_unpk2 = 0x41,
    op_unpk2u = 0x61,
};

enum class scp_custom_op_t {
    op_lcl = 0x10,
    op_rel = 0x11,
};

enum class csr_perm_t {
    ro = 0b00,
    rw = 0b01,
    warl_unimp = 0b10
};

enum class rf_names_t { mode_x, mode_abi };

// caches
enum class access_t { read, write };
enum class scp_mode_t { m_none, m_lcl, m_rel };
// success always 0, fail 1 for now, use values >0 for error codes if needed
enum class scp_status_t { success, fail };
enum class speculative_t { enter, exit_commit, exit_flush };

// branches
enum class b_dir_t { backward, forward};
enum class bp_t {sttc, bimodal, local, global, gselect, gshare, ideal, combined,
                 _count };

// dasm
struct dasm_str {
    std::ostringstream asm_ss;
    std::string asm_str;
    std::string op;
};

// RF
struct reg_pair {
    uint32_t a;
    uint32_t b;
};

// CSRs
struct CSR {
    const char* name;
    uint32_t value;
    const csr_perm_t perm;
    CSR() : name(""), value(0), perm(csr_perm_t::ro) {} // FIXME
    CSR(const char* name, uint32_t value, const csr_perm_t perm) :
        name(name), value(value), perm(perm) {}
};

struct CSR_entry {
    const uint16_t csr_addr;
    const char* csr_name;
    const csr_perm_t perm;
    const uint32_t boot_val;
};

// CLI options
struct logging_pc_t {
    public:
        uint32_t start;
        uint32_t stop;
        uint32_t single_match_num;
        uint64_t inst_cnt;
    private:
        uint32_t current_match = 0;
        bool ran_once = false;
    public:
        bool should_start() {
            if (!single_match_num) return true;
            current_match++;
            if (!ran_once && (current_match == single_match_num)) {
                ran_once = true;
                return true;
            }
            return false;
        }
        bool should_start(uint32_t pc) {
            if (!(pc == start)) return false;
            return should_start();
        }
        bool should_stop(uint32_t pc) {
            if (pc == stop) return true;
            return false;
        }
};

struct cfg_t {
    logging_pc_t log_pc;
    rf_names_t rf_names;
    bool dump_all_regs;
};

struct hw_cfg_t {
    // caches
    uint32_t icache_sets;
    uint32_t icache_ways;
    uint32_t dcache_sets;
    uint32_t dcache_ways;
    uint32_t roi_start;
    uint32_t roi_size;
    // branch predictors
    bp_t bp_active;
    // comvined predictor config
    bp_t bpc_1;
    bp_t bpc_2;
};

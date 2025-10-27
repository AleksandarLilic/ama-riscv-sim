#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cmath>
#include <bitset>

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
    op_mul = 0x0,
    op_mulh = 0x1,
    op_mulhsu = 0x2,
    op_mulhu = 0x3,
    op_div = 0x4,
    op_divu = 0x5,
    op_rem = 0x6,
    op_remu = 0x7,
};

enum class alu_r_zbb_op_t {
    op_max = 0x6,
    op_maxu = 0x7,
    op_min = 0x4,
    op_minu = 0x5,
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
    op_add16 = 0x0,
    op_add8 = 0x1,
    op_sub16 = 0x2,
    op_sub8 = 0x3,
    op_mul16 = 0x04,
    op_mul16u = 0x24,
    op_mul8 = 0x05,
    op_mul8u = 0x25,
    op_dot16 = 0x6,
    op_dot8 = 0x7,
    op_dot4 = 0x46,
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
    ro = 0b00, // read-only
    rw = 0b01, // read-write
    warl = 0b10, // write-any-read-legal
    warl_unimp = 0b11, // warl unimplemented -> always returns 0
};

enum class rf_names_t { mode_x, mode_abi };
enum class mem_op_t { read, write };
enum class b_dir_t { backward, forward };

// profilers
enum class profiler_source_t { inst, clock };

enum class dmem_size_t {
    lb, lh, lw, ld,
    sb, sh, sw, sd,
    no_access
};

struct clock_source_t {
    private:
        uint64_t pr = 0;
        uint64_t cr = 0;
        uint64_t diff = 0;
        uint64_t mtime = 0; // FIXME: ugly workaround
    public:
        void update(uint64_t sample, uint64_t mtime) {
            pr = cr;
            cr = sample;
            diff = cr - pr;
            this->mtime = mtime;
        }
        uint64_t get_cr() { return cr; }
        uint64_t get_diff() { return diff; }
        uint64_t get_mtime() { return mtime; }
};

struct symbol_map_entry_t {
    uint16_t idx;
    std::string name;
};

struct symbol_lut_entry_t {
    uint32_t pc;
    std::string name;
};

struct symbol_tracking_t {
    std::vector<uint16_t> idx_callstack;
    std::vector<uint16_t> idx_callstack_prev;
    uint32_t fallthrough_pc;
    bool updated;
};

// perf events
enum class perf_event_t {
    inst,
    #ifdef DPI
    cycle,
    #endif
    branch,
    mem,
    //mem_load,
    //mem_store,
    simd,
    #if defined(HW_MODELS_EN) || defined(DPI)
    //cache_reference, // only applicable for multi-level caches
    //cache_miss, // only applicable for multi-level caches
    icache_reference,
    icache_miss,
    dcache_reference,
    dcache_miss,
    bp_mispredict,
    #endif
    _count
};

static const
std::array<std::string, static_cast<uint32_t>(perf_event_t::_count)>
perf_event_names = {
    "inst",
    #ifdef DPI
    "cycle",
    #endif
    "branch",
    "mem",
    //"mem_load",
    //"mem_store",
    "simd",
    #if defined(HW_MODELS_EN) || defined(DPI)
    //"cache_reference",
    //"cache_miss",
    "icache_reference",
    "icache_miss",
    "dcache_reference",
    "dcache_miss",
    "bp_mispredict",
    #endif
};

// dasm
struct dasm_str {
    public:
        std::ostringstream asm_ss;
        std::ostringstream simd_ss;
        std::ostringstream simd_a;
        std::ostringstream simd_b;
        std::ostringstream simd_c;
        std::string asm_str;
        std::string op;
    public:
        void finish_inst() {
            asm_ss << simd_ss.str();
            asm_str = asm_ss.str();
        }
        void clear_str() {
            asm_ss.str("");
            simd_ss.str("");
        }
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
struct prof_pc_t {
    public:
        uint32_t start;
        uint32_t stop;
        uint32_t single_match_num;
        uint64_t inst_cnt;
        bool exit_on_prof_stop;
    private:
        uint32_t current_match = 0;
        bool ran_once = false;
    public:
        bool should_start() {
            if (!single_match_num) return true; // dc if not single matching
            current_match++;
            if (current_match == single_match_num) { // count matched
                ran_once = true;
                return true;
            }
            return false; // single matching, but count not yet matched
        }
        bool should_start(uint32_t pc) {
            if (!(pc == start)) return false;
            return should_start();
        }
        bool should_stop(uint32_t pc) {
            if (pc == stop) return true;
            return false;
        }
        bool should_exit() {
            if (!exit_on_prof_stop) return false; // dc if inactive
            if (!single_match_num) return true;// exit on first stop if no match
            if (ran_once) return true; // exit if single match and ran once
            return false; // single matching, but not run yet
        }
        bool should_exit(uint32_t pc) {
            if (pc != stop) return false; // dc if not at stop pc
            return should_exit();
        }
};

struct cfg_t {
    prof_pc_t prof_pc;
    rf_names_t rf_names;
    uint32_t mem_dump_start;
    uint32_t mem_dump_size;
    perf_event_t perf_event;
    uint64_t run_insts;
    bool prof_trace;
    bool rf_usage;
    bool log;
    bool log_always;
    bool log_state;
    bool log_hw_models;
    bool show_state;
    bool exit_on_trap;
    bool sink_uart;
    std::string out_dir;
};

struct logging_flags_t {
    bool en;
    bool act;
    bool always;
    bool state;
    void activate(bool in) { act = en && (in || always); }
};

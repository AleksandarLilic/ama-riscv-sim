#pragma once

#include <algorithm>
#include <cassert>

#include "defines.h"

#define PROF_JSON_ENTRY(name, count) \
    "\"" << name << "\"" << ": {\"count\": " << count << "},"

#define PROF_JSON_ENTRY_J(name, count_taken, count_taken_fwd, \
                          count_not_taken, count_not_taken_fwd) \
    "\"" << name << "\"" << ": {\"count\": " << count_taken + count_not_taken \
    << ", \"breakdown\": {" \
    << "\"taken\": " << count_taken << ", " \
    << "\"taken_fwd\": " << count_taken_fwd << ", " \
    << "\"taken_bwd\": " << count_taken - count_taken_fwd << ", " \
    << "\"not_taken\": " << count_not_taken << ", " \
    << "\"not_taken_fwd\": " << count_not_taken_fwd << ", " \
    << "\"not_taken_bwd\": " << count_not_taken - count_not_taken_fwd \
    <<  "}},"

enum class opc_g {
    // RV32I
    // reg-reg
    i_add, i_sub, i_sll, i_srl, i_sra,
    i_slt, i_sltu, i_xor, i_or, i_and,
    // imm
    i_nop, i_addi, i_slli, i_srli, i_srai,
    i_slti, i_sltiu, i_xori, i_ori, i_andi,
    // mem
    i_lb, i_lh, i_lw, i_lbu, i_lhu, i_sb, i_sh, i_sw, i_fence_i, i_fence,
    // upper imm
    i_lui, i_auipc,
    // system
    i_ecall, i_ebreak,
    // hints
    i_hint,

    // custom
    i_fma16, i_fma8, i_fma4,
    // custom mem
    i_scp_lcl, i_scp_rel,

    // Zicsr extension
    i_csrrw, i_csrrs, i_csrrc, i_csrrwi, i_csrrsi, i_csrrci,

    // M extension
    i_mul, i_mulh, i_mulhsu, i_mulhu, i_div, i_divu, i_rem, i_remu,

    // C extension
    // reg-reg
    i_c_add, i_c_mv, i_c_and, i_c_or, i_c_xor, i_c_sub,
    // reg-imm
    i_c_addi, i_c_addi16sp, i_c_addi4spn, i_c_andi, i_c_srli, i_c_slli,
    i_c_srai, i_c_nop,
    // sp-based mem, reg-based mem
    i_c_lwsp, i_c_swsp, i_c_lw, i_c_sw,
    // upper imm
    i_c_li, i_c_lui,
    // system
    i_c_ebreak,

    // end of enum
    _count
};

// TODO: store PC for each branch?
enum class opc_j {
    // RV32I
    i_beq, i_bne, i_blt, i_bge, i_bltu, i_bgeu, i_jalr, i_jal,
    // C extension
    i_c_j, i_c_jal, i_c_jr, i_c_jalr, i_c_beqz, i_c_bnez,
    // end of enum
    _count
};

enum class reg_use_t { rd, rs1, rs2 };

struct inst_prof_g {
    std::string name;
    uint32_t count;
};

struct inst_prof_j {
    std::string name;
    uint32_t count_taken;
    uint32_t count_taken_fwd;
    uint32_t count_not_taken;
    uint32_t count_not_taken_fwd;
};

// TODO: add instruction dasm to the profiler as another entry?
struct trace_entry {
    uint32_t pc;
    uint32_t inst_size;
    uint32_t dmem;
    uint32_t dmem_size;
    uint32_t sp;
};

class profiler{
    public:
        trace_entry te;
        bool active;

    private:
        std::string log_name;
        std::ofstream ofs;
        uint64_t inst_cnt;
        uint32_t inst;
        std::vector<trace_entry> trace;
        std::array<inst_prof_g, TO_U32(opc_g::_count)> prof_g_arr;
        std::array<inst_prof_j, TO_U32(opc_j::_count)> prof_j_arr;
        std::array<std::array<uint32_t, 3>, 32> prof_reg_hist = {0};

    public:
        profiler() = delete;
        profiler(std::string log_name);
        void new_inst(uint32_t inst);
        void log_inst(opc_g opc);
        void log_inst(opc_j opc, bool taken, b_dir_t direction);
        void log();
        void log_reg_use(reg_use_t reg_use, uint8_t reg);
        void finish() { log_to_file(); }

    private:
        void log_to_file();
        void info(uint32_t profiled_inst_cnt, uint32_t max_sp);
        void rst_te() { te = {0, 0, 0, 0, 0}; }
};

#pragma once

#include "defines.h"

#include <cassert>

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
    // trap
    i_mret, i_wfi,

    // custom al
    i_add16, i_add8, i_sub16, i_sub8,
    i_mul16, i_mul16u, i_mul8, i_mul8u,
    i_dot16, i_dot8, i_dot4,
    // custom mem
    i_unpk16, i_unpk16u, i_unpk8, i_unpk8u,
    i_unpk4, i_unpk4u, i_unpk2, i_unpk2u,
    // custom hints
    i_scp_lcl, i_scp_rel,

    // Zicsr extension
    i_csrrw, i_csrrs, i_csrrc, i_csrrwi, i_csrrsi, i_csrrci,

    // M extension
    i_mul, i_mulh, i_mulhsu, i_mulhu, i_div, i_divu, i_rem, i_remu,

    // Zbb extension
    i_max, i_maxu, i_min, i_minu,

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

struct stack_access_t {
    private:
        uint64_t load = 0;
        uint64_t store = 0;
    public:
        void loading(bool in_range) { load += in_range; }
        void storing(bool in_range) { store += in_range; }
        uint64_t total() { return load + store; }
        uint64_t get_load() { return load; }
        uint64_t get_store() { return store; }
};

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
    uint64_t sample_cnt; // inst count in isa sim, cycle count in rtl sim
    uint32_t pc;
    uint32_t dmem;
    uint32_t sp;
    uint8_t ic_hm;
    uint8_t dc_hm;
    uint8_t bp_hm;
    uint8_t inst_size;
    uint8_t dmem_size;
};

struct cnt_t {
    public:
        uint32_t tot = 0;
        uint32_t rest = 0;
        uint32_t nop = 0;
        uint32_t branch = 0;
        uint32_t jump = 0;
        uint32_t load = 0;
        uint32_t store = 0;
        uint32_t mem = 0;
        uint32_t mul = 0;
        uint32_t div = 0;
        uint32_t al = 0;
        uint32_t zbb = 0;
        uint32_t dot_c = 0;
        uint32_t al_c = 0;
        uint32_t mul_c = 0;
        uint32_t unpk_c = 0;
        uint32_t scp_c = 0;

    public:
        void find_mem() { mem = load + store; }
        void find_rest() {
            rest = tot - nop - branch - jump - mem -
                   mul - div - al - zbb -
                   dot_c - al_c - mul_c - unpk_c - scp_c;
        }
        float_t get_perc(uint32_t count) {
            if (count == 0 || tot == 0) return 0.0;
            return 100.0 * count / tot;
        }
};

struct perc_t {
    public:
        float_t tot = 0.0;
        float_t rest = 0.0;
        float_t nop = 0.0;
        float_t branch = 0.0;
        float_t jump = 0.0;
        float_t load = 0.0;
        float_t store = 0.0;
        float_t mem = 0.0;
        float_t mul = 0.0;
        float_t div = 0.0;
        float_t al = 0.0;
        float_t zbb = 0.0;
        float_t dot_c = 0.0;
        float_t al_c = 0.0;
        float_t mul_c = 0.0;
        float_t unpk_c = 0.0;
        float_t scp_c = 0.0;
};

struct sparsity_cnt_t {
    uint64_t total = 0;
    uint64_t sparse = 0;
    float_t get_perc() {
        if (total == 0) return 0.0;
        return 100.0 * sparse / total;
    }
};

class profiler {
    public:
        trace_entry te;
        bool active;

    private:
        std::string out_dir;
        profiler_source_t prof_src;
        std::ofstream ofs;
        uint64_t inst_cnt_exec;
        stack_access_t stack_access;
        uint32_t inst;
        std::vector<trace_entry> trace;
        std::array<inst_prof_g, TO_U32(opc_g::_count)> prof_g_arr;
        std::array<inst_prof_j, TO_U32(opc_j::_count)> prof_j_arr;
        std::array<std::array<uint32_t, 3>, 32> prof_rf_usage = {0};
        sparsity_cnt_t sparsity_cnt;
        bool trace_en;
        bool rf_usage;

    public:
        profiler() = delete;
        profiler(std::string out_dir, profiler_source_t prof_src);
        void new_inst(uint32_t inst) { this->inst = inst; }
        void add_te();
        void log_inst(opc_g opc, uint64_t inc);
        void log_inst(opc_j opc, bool taken, b_dir_t b_dir, uint64_t inc);
        void log_reg_use(reg_use_t reg_use, uint8_t reg);
        void log_stack_access_load(bool in_range) {
            if (active) stack_access.loading(in_range);
        }
        void log_stack_access_store(bool in_range) {
            if (active) stack_access.storing(in_range);
        }
        void set_prof_flags(bool trace_en, bool rf_usage) {
            this->trace_en = trace_en;
            this->rf_usage = rf_usage;
        }
        void finish() { log_to_file_and_print(); }
        void log_sparsity(bool sparse) {
            if (!active) return;
            sparsity_cnt.total++;
            sparsity_cnt.sparse += sparse;
        }

    private:
        void log_to_file_and_print();
        void rst_te() { te = {0, 0, 0, 0, 0, 0, 0, 0, 0}; }

    private:
        // all compressed instructions
        static constexpr std::array<opc_g, 21> comp_opcs_alu = {
            opc_g::i_c_add, opc_g::i_c_mv, opc_g::i_c_and, opc_g::i_c_or,
            opc_g::i_c_xor, opc_g::i_c_sub,
            opc_g::i_c_addi, opc_g::i_c_addi16sp, opc_g::i_c_addi4spn,
            opc_g::i_c_andi, opc_g::i_c_srli, opc_g::i_c_slli, opc_g::i_c_srai,
            opc_g::i_c_nop,
            opc_g::i_c_lwsp, opc_g::i_c_swsp, opc_g::i_c_lw, opc_g::i_c_sw,
            opc_g::i_c_li, opc_g::i_c_lui,
            opc_g::i_c_ebreak
        };

        static constexpr std::array<opc_j, 6> comp_opcs_j = {
            opc_j::i_c_j, opc_j::i_c_jal, opc_j::i_c_jr, opc_j::i_c_jalr,
            opc_j::i_c_beqz, opc_j::i_c_bnez,
        };

        // per type breakdowns
        static constexpr std::array<opc_j, 8> branch_opcs = {
            opc_j::i_beq, opc_j::i_bne, opc_j::i_blt,
            opc_j::i_bge, opc_j::i_bltu, opc_j::i_bgeu,
            // compressed
            opc_j::i_c_beqz, opc_j::i_c_bnez
        };

        static constexpr std::array<opc_j, 6> jump_opcs = {
            opc_j::i_jalr, opc_j::i_jal,
            // compressed
            opc_j::i_c_j, opc_j::i_c_jal, opc_j::i_c_jr, opc_j::i_c_jalr,
        };

        static constexpr std::array<opc_g, 7> load_opcs = {
            opc_g::i_lb, opc_g::i_lh, opc_g::i_lw, opc_g::i_lbu, opc_g::i_lhu,
            // compressed
            opc_g::i_c_lw, opc_g::i_c_lwsp
        };

        static constexpr std::array<opc_g, 5> store_opcs = {
            opc_g::i_sb, opc_g::i_sh, opc_g::i_sw,
            // compressed
            opc_g::i_c_sw, opc_g::i_c_swsp
        };

        static constexpr std::array<opc_g, 4> mul_opcs = {
            opc_g::i_mul, opc_g::i_mulh, opc_g::i_mulhsu, opc_g::i_mulhu,
        };

        static constexpr std::array<opc_g, 4> div_opcs = {
            opc_g::i_div, opc_g::i_divu, opc_g::i_rem, opc_g::i_remu,
        };

        static constexpr std::array<opc_g, 36> alu_opcs = {
            opc_g::i_add, opc_g::i_sub,
            opc_g::i_sll, opc_g::i_srl, opc_g::i_sra,
            opc_g::i_slt, opc_g::i_sltu,
            opc_g::i_xor, opc_g::i_or, opc_g::i_and,
            opc_g::i_addi,
            opc_g::i_slli, opc_g::i_srli, opc_g::i_srai,
            opc_g::i_slti, opc_g::i_sltiu,
            opc_g::i_xori, opc_g::i_ori, opc_g::i_andi,
            opc_g::i_lui, opc_g::i_auipc,
            // compressed
            opc_g::i_c_add, opc_g::i_c_sub, opc_g::i_c_mv,
            opc_g::i_c_addi16sp, opc_g::i_c_addi4spn,
            opc_g::i_c_and, opc_g::i_c_or, opc_g::i_c_xor,
            opc_g::i_c_addi,
            opc_g::i_c_andi, opc_g::i_c_srli, opc_g::i_c_slli, opc_g::i_c_srai,
            opc_g::i_c_li, opc_g::i_c_lui
        };

        static constexpr std::array<opc_g, 4> zbb_opcs = {
            opc_g::i_max, opc_g::i_maxu, opc_g::i_min, opc_g::i_minu,
        };

        static constexpr std::array<opc_g, 3> dot_c_opcs = {
            opc_g::i_dot16, opc_g::i_dot8, opc_g::i_dot4,
        };

        static constexpr std::array<opc_g, 4> al_c_opcs = {
            opc_g::i_add16, opc_g::i_add8, opc_g::i_sub16, opc_g::i_sub8,
        };

        static constexpr std::array<opc_g, 4> mul_c_opcs = {
            opc_g::i_mul16, opc_g::i_mul16u, opc_g::i_mul8, opc_g::i_mul8u,
        };

        static constexpr std::array<opc_g, 8> unpk_c_opcs = {
            opc_g::i_unpk16, opc_g::i_unpk16u,
            opc_g::i_unpk8, opc_g::i_unpk8u,
            opc_g::i_unpk4, opc_g::i_unpk4u,
            opc_g::i_unpk2, opc_g::i_unpk2u,
        };

        static constexpr std::array<opc_g, 2> scp_c_opcs = {
            opc_g::i_scp_lcl, opc_g::i_scp_rel,
        };
};

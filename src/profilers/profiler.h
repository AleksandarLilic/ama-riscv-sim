#pragma once

#include "defines.h"
#include "utils.h"
#include "hw_model_types.h"

#include <cassert>

#define PROF_JSON_ENTRY(name, count) \
    INDENT << "\"" << name << "\"" << ": {\"count\": " << count << "},"

#define PROF_JSON_ENTRY_B(name, count_taken, count_taken_fwd, \
                          count_not_taken, count_not_taken_fwd) \
    INDENT << "\"" << name << "\"" \
    << ": {\"count\": " << count_taken + count_not_taken \
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

    // custom
    // add/sub
    i_add16, i_add8, i_sub16, i_sub8,
    // add/sub sat
    i_qadd16, i_qadd16u, i_qadd8, i_qadd8u,
    i_qsub16, i_qsub16u, i_qsub8, i_qsub8u,
    // widening mul
    i_wmul16, i_wmul16u, i_wmul8, i_wmul8u,
    // dot
    i_dot16, i_dot16u, i_dot8, i_dot8u, i_dot4, i_dot4u, i_dot2, i_dot2u,
    // min/max
    i_min16, i_min16u, i_min8, i_min8u,
    i_max16, i_max16u, i_max8, i_max8u,
    // shift
    i_slli16, i_slli8, i_srli16, i_srli8, i_srai16, i_srai8,

    // custom data fmt
    // widen
    i_widen16, i_widen16u, i_widen8, i_widen8u,
    i_widen4, i_widen4u, i_widen2, i_widen2u,
    // narrow truncating
    i_narrow32, i_narrow16, i_narrow8, i_narrow4,
    // narrow saturating
    i_qnarrow32, i_qnarrow32u, i_qnarrow16, i_qnarrow16u,
    i_qnarrow8, i_qnarrow8u, i_qnarrow4, i_qnarrow4u,
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
enum class opc_b {
    // RV32I
    i_beq, i_bne, i_blt, i_bge, i_bltu, i_bgeu, i_jalr, i_jal,
    // C extension
    i_c_j, i_c_jal, i_c_jr, i_c_jalr, i_c_beqz, i_c_bnez,
    // end of enum
    _count
};

enum class reg_use_t { rd, rdp, rs1, rs2, rs3, _count };

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

struct inst_prof_b {
    std::string name;
    uint32_t count_taken;
    uint32_t count_taken_fwd;
    uint32_t count_not_taken;
    uint32_t count_not_taken_fwd;
};

// TODO: add instruction dasm to the profiler as another entry?
struct trace_entry {
    public:
        uint64_t sample_cnt; // inst count in isa sim, cycle count in rtl sim
        // instruction specific (retired in RTL)
        uint32_t inst;
        uint32_t pc;
        uint32_t next_pc;
        uint32_t dmem;
        uint32_t sp;
        uint8_t taken;
        uint8_t inst_size;
        uint8_t dmem_size;
        // current sample specific (same thing in ISA sim as current inst)
        uint8_t ic_hm;
        uint8_t dc_hm;
        uint8_t bp_hm;
        #ifdef DPI
        uint8_t ct_imem_core;
        uint8_t ct_imem_mem;
        uint8_t ct_dmem_core_r;
        uint8_t ct_dmem_core_w;
        uint8_t ct_dmem_mem_r;
        uint8_t ct_dmem_mem_w;
        #endif

    public:
        void rst() {
            sample_cnt = 0;
            inst = 0;
            pc = 0;
            next_pc = 0;
            dmem = 0;
            sp = 0;
            taken = 0;
            inst_size = 0;
            dmem_size = TO_U8(dmem_size_t::no_access);
            ic_hm = TO_U8(hw_status_t::none);
            dc_hm = TO_U8(hw_status_t::none);
            bp_hm = TO_U8(hw_status_t::none);
            #ifdef DPI
            ct_imem_core = 0;
            ct_imem_mem = 0;
            ct_dmem_core_r =  0;
            ct_dmem_core_w = 0;
            ct_dmem_mem_r = 0;
            ct_dmem_mem_w = 0;
            #endif
        }
};

struct cnt_t {
    public:
        uint32_t tot = 0;
        uint32_t rest = 0;
        uint32_t nop = 0;
        uint32_t branch = 0;
        uint32_t jal = 0;
        uint32_t jalr = 0;
        uint32_t load = 0;
        uint32_t store = 0;
        uint32_t mem = 0;
        uint32_t mul = 0;
        uint32_t div = 0;
        uint32_t alu = 0;
        uint32_t zbb = 0;
        uint32_t dot_c = 0;
        uint32_t alu_c = 0;
        uint32_t wmul_c = 0;
        uint32_t widen_c = 0;
        uint32_t narrow_c = 0;
        uint32_t scp_c = 0;

    public:
        void find_mem() { mem = load + store; }
        void find_rest() {
            rest = (
                tot - nop - branch - jal - jalr - mem -
                mul - div - alu - zbb -
                dot_c - alu_c - wmul_c - widen_c - narrow_c - scp_c
            );
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
        float_t jal = 0.0;
        float_t jalr = 0.0;
        float_t load = 0.0;
        float_t store = 0.0;
        float_t mem = 0.0;
        float_t mul = 0.0;
        float_t div = 0.0;
        float_t alu = 0.0;
        float_t zbb = 0.0;
        float_t dot_c = 0.0;
        float_t alu_c = 0.0;
        float_t wmul_c = 0.0;
        float_t widen_c = 0.0;
        float_t narrow_c = 0.0;
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

enum class sparsity_t {
    any, alu, mem_l, mem_s, simd_dot, simd_alu, simd_data_fmt, _count
};

static constexpr std::array<const char*, TO_U32(sparsity_t::_count)>
    sparsity_cnt_names =
    {{
        "ANY", "ALU", "MEM_L", "MEM_S", "SIMD_DOT", "SIMD_ALU", "SIMD_DATA_FMT"
    }};

class profiler {
    public:
        trace_entry te;
        bool active;

    private:
        std::string out_dir;
        profiler_source_t prof_src;
        std::ofstream ofs;
        uint64_t inst_cnt_prof;
        stack_access_t stack_access;
        uint32_t inst;
        #ifdef PROF_MEM_USAGE
        logging_resource logres;
        std::pmr::vector<trace_entry> trace{ &logres };
        #else
        std::vector<trace_entry> trace;
        #endif
        std::array<inst_prof_g, TO_U32(opc_g::_count)> prof_g_arr;
        std::array<inst_prof_b, TO_U32(opc_b::_count)> prof_b_arr;
        std::array<std::array<uint64_t, TO_U32(reg_use_t::_count)>, 32>
            prof_rf_usage = {0};
        std::array<sparsity_cnt_t, TO_U32(sparsity_t::_count)> sparsity_cnt;
        bool trace_en;
        bool rf_usage;
        uint32_t min_sp = BASE_ADDR + MEM_SIZE; // add offset

    public:
        profiler() = delete;
        profiler(std::string out_dir, profiler_source_t prof_src);
        void new_inst(uint32_t inst) { this->inst = inst; }
        void add_te();
        void track_sp(const uint32_t sp);
        void log_inst(opc_g opc, uint64_t inc);
        void log_inst(opc_b opc, bool taken, b_dir_t b_dir, uint64_t inc);
        void log_reg_use(reg_use_t reg_use, uint8_t reg);
        void log_sparsity(bool sparse, sparsity_t stype) {
            if (!active) return;
            sparsity_cnt[TO_U32(stype)].total++;
            sparsity_cnt[TO_U32(stype)].sparse += sparse;
        }
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
        void finish(bool silent) { log_to_file_and_print(silent); }

    private:
        void log_to_file_and_print(bool silent);

    private:
        // all compressed instructions
        static constexpr std::array comp_opcs_alu = {
            opc_g::i_c_add, opc_g::i_c_mv, opc_g::i_c_and, opc_g::i_c_or,
            opc_g::i_c_xor, opc_g::i_c_sub,
            opc_g::i_c_addi, opc_g::i_c_addi16sp, opc_g::i_c_addi4spn,
            opc_g::i_c_andi, opc_g::i_c_srli, opc_g::i_c_slli, opc_g::i_c_srai,
            opc_g::i_c_nop,
            opc_g::i_c_lwsp, opc_g::i_c_swsp, opc_g::i_c_lw, opc_g::i_c_sw,
            opc_g::i_c_li, opc_g::i_c_lui,
            opc_g::i_c_ebreak
        };

        static constexpr std::array comp_opcs_b = {
            opc_b::i_c_j, opc_b::i_c_jal, opc_b::i_c_jr, opc_b::i_c_jalr,
            opc_b::i_c_beqz, opc_b::i_c_bnez,
        };

        // per type breakdowns
        // direct, conditinal branches
        static constexpr std::array branch_opcs = {
            opc_b::i_beq, opc_b::i_bne, opc_b::i_blt,
            opc_b::i_bge, opc_b::i_bltu, opc_b::i_bgeu,
            // compressed
            opc_b::i_c_beqz, opc_b::i_c_bnez
        };

        // direct, unconditional branches
        static constexpr std::array jal_opcs = {
            opc_b::i_jal,
            // compressed
            opc_b::i_c_j, opc_b::i_c_jal,
        };

        // indirect, unconditional branches
        static constexpr std::array jalr_opcs = {
            opc_b::i_jalr,
            // compressed
            opc_b::i_c_jr, opc_b::i_c_jalr,
        };

        static constexpr std::array load_opcs = {
            opc_g::i_lb, opc_g::i_lh, opc_g::i_lw, opc_g::i_lbu, opc_g::i_lhu,
            // compressed
            opc_g::i_c_lw, opc_g::i_c_lwsp
        };

        static constexpr std::array store_opcs = {
            opc_g::i_sb, opc_g::i_sh, opc_g::i_sw,
            // compressed
            opc_g::i_c_sw, opc_g::i_c_swsp
        };

        static constexpr std::array mul_opcs = {
            opc_g::i_mul, opc_g::i_mulh, opc_g::i_mulhsu, opc_g::i_mulhu,
        };

        static constexpr std::array div_opcs = {
            opc_g::i_div, opc_g::i_divu, opc_g::i_rem, opc_g::i_remu,
        };

        static constexpr std::array alu_opcs = {
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

        static constexpr std::array zbb_opcs = {
            opc_g::i_max, opc_g::i_maxu, opc_g::i_min, opc_g::i_minu,
        };

        static constexpr std::array dot_c_opcs = {
            opc_g::i_dot16, opc_g::i_dot8, opc_g::i_dot4, opc_g::i_dot2,
            opc_g::i_dot16u, opc_g::i_dot8u, opc_g::i_dot4u, opc_g::i_dot2u,
        };

        static constexpr std::array alu_c_opcs = {
            opc_g::i_add16, opc_g::i_add8, opc_g::i_sub16, opc_g::i_sub8,
            opc_g::i_qadd16, opc_g::i_qadd16u, opc_g::i_qadd8, opc_g::i_qadd8u,
            opc_g::i_qsub16, opc_g::i_qsub16u, opc_g::i_qsub8, opc_g::i_qsub8u,
            opc_g::i_min16, opc_g::i_min16u, opc_g::i_min8, opc_g::i_min8u,
            opc_g::i_max16, opc_g::i_max16u, opc_g::i_max8, opc_g::i_max8u,
            opc_g::i_slli16, opc_g::i_slli8,
            opc_g::i_srli16, opc_g::i_srli8, opc_g::i_srai16, opc_g::i_srai8,
        };

        static constexpr std::array mul_c_opcs = {
            opc_g::i_wmul16, opc_g::i_wmul16u, opc_g::i_wmul8, opc_g::i_wmul8u,
        };

        static constexpr std::array widen_c_opcs = {
            opc_g::i_widen16, opc_g::i_widen16u,
            opc_g::i_widen8, opc_g::i_widen8u,
            opc_g::i_widen4, opc_g::i_widen4u,
            opc_g::i_widen2, opc_g::i_widen2u,
        };

        static constexpr std::array narrow_c_opcs = {
            // truncating
            opc_g::i_qnarrow32, opc_g::i_qnarrow16,
            opc_g::i_qnarrow8, opc_g::i_qnarrow4,
            // saturating
            opc_g::i_qnarrow32, opc_g::i_qnarrow32u,
            opc_g::i_qnarrow16, opc_g::i_qnarrow16u,
            opc_g::i_qnarrow8, opc_g::i_qnarrow8u,
            opc_g::i_qnarrow4, opc_g::i_qnarrow4u,
        };

        static constexpr std::array scp_c_opcs = {
            opc_g::i_scp_lcl, opc_g::i_scp_rel,
        };
};

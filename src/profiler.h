#pragma once

#include "defines.h"

#define JSON_ENTRY(name, count) \
    "\"" << name << "\"" << ": {\"count\": " << count << "}, "

#define JSON_ENTRY_J(name, count_taken, count_taken_fwd, \
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

enum class b_dir_t {
    backward, forward
};

enum class al_type_t {
    reg, imm
};

enum class opc_al {
    i_nop, i_add, i_sub, i_sll, i_srl, i_sra,
    i_slt, i_sltu, i_xor, i_or, i_and, _count
};

enum class opc_mem {
    i_lb, i_lh, i_lw, i_lbu, i_lhu, i_sb, i_sh, i_sw, i_fence_i, _count
};

enum class opc_upp {
    i_lui, i_auipc, _count
};

enum class opc_sys {
    i_ecall, i_ebreak, _count
};

enum class opc_csr {
    i_csrrw, i_csrrs, i_csrrc, i_csrrwi, i_csrrsi, i_csrrci, _count
};

// TODO: store PC for each branch?
enum class opc_j {
    i_beq, i_bne, i_blt, i_bge, i_bltu, i_bgeu, i_jalr, i_jal, _count
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

class profiler{
    private:
        std::ofstream out_stream;
        uint32_t inst_cnt;
        uint32_t inst;
        std::array<uint32_t, (MEM_SIZE>>2)> pc_hist;
        al_type_t al_type;
        std::array<inst_prof_g, static_cast<uint32_t>(opc_al::_count)>
            prof_alr_arr;
        std::array<inst_prof_g, static_cast<uint32_t>(opc_al::_count)>
            prof_ali_arr;
        std::array<inst_prof_g, static_cast<uint32_t>(opc_mem::_count)>
            prof_mem_arr;
        std::array<inst_prof_g, static_cast<uint32_t>(opc_upp::_count)>
            prof_upp_arr;
        std::array<inst_prof_g, static_cast<uint32_t>(opc_sys::_count)>
            prof_sys_arr;
        std::array<inst_prof_g, static_cast<uint32_t>(opc_csr::_count)>
            prof_csr_arr;
        std::array<inst_prof_j, static_cast<uint32_t>(opc_j::_count)>
            prof_j_arr;
        std::array<inst_prof_g, static_cast<uint32_t>(opc_al::_count)>
            *prof_al_arr_ptrs[2] = {&prof_alr_arr, &prof_ali_arr};
        
        std::string log_name;

    public:
        profiler() = delete;
        profiler(std::string log_name);
        void set_al_type(al_type_t type) { al_type = type; }
        void new_inst(uint32_t inst) { this->inst = inst; inst_cnt++; }
        void log_inst(opc_al opc);
        void log_inst(opc_mem opc);
        void log_inst(opc_upp opc);
        void log_inst(opc_sys opc);
        void log_inst(opc_csr opc);
        void log_inst(opc_j opc, bool taken, b_dir_t direction);
        void log_pc(uint32_t pc);
        void log_to_file();
    
    private:
        void info(uint32_t inst_cnt, uint32_t profiled_inst_cnt);
};

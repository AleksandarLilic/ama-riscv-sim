#include "profiler.h"

profiler::profiler(std::string log_name) {
    inst_cnt = 0;
    this->log_name = log_name;
    pc_cnt.fill(0);

    prof_alr_arr[static_cast<uint32_t>(opc_al::i_add)] = {"add", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al::i_sub)] = {"sub", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al::i_sll)] = {"sll", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al::i_srl)] = {"srl", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al::i_sra)] = {"sra", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al::i_slt)] = {"slt", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al::i_sltu)] = {"sltu", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al::i_xor)] = {"xor", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al::i_or)] = {"or", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al::i_and)] = {"and", 0};

    prof_ali_arr[static_cast<uint32_t>(opc_al::i_nop)] = {"nop", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al::i_add)] = {"addi", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al::i_sll)] = {"slli", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al::i_srl)] = {"srli", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al::i_sra)] = {"srai", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al::i_slt)] = {"slti", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al::i_sltu)] = {"sltiu", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al::i_xor)] = {"xori", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al::i_or)] = {"ori", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al::i_and)] = {"andi", 0};

    prof_mem_arr[static_cast<uint32_t>(opc_mem::i_lb)] = {"lb", 0};
    prof_mem_arr[static_cast<uint32_t>(opc_mem::i_lh)] = {"lh", 0};
    prof_mem_arr[static_cast<uint32_t>(opc_mem::i_lw)] = {"lw", 0};
    prof_mem_arr[static_cast<uint32_t>(opc_mem::i_lbu)] = {"lbu", 0};
    prof_mem_arr[static_cast<uint32_t>(opc_mem::i_lhu)] = {"lhu", 0};
    prof_mem_arr[static_cast<uint32_t>(opc_mem::i_sb)] = {"sb", 0};
    prof_mem_arr[static_cast<uint32_t>(opc_mem::i_sh)] = {"sh", 0};
    prof_mem_arr[static_cast<uint32_t>(opc_mem::i_sw)] = {"sw", 0};
    prof_mem_arr[static_cast<uint32_t>(opc_mem::i_fence_i)] = {"fence.i", 0};

    prof_upp_arr[static_cast<uint32_t>(opc_upp::i_lui)] = {"lui", 0};
    prof_upp_arr[static_cast<uint32_t>(opc_upp::i_auipc)] = {"auipc", 0};

    prof_sys_arr[static_cast<uint32_t>(opc_sys::i_ecall)] = {"ecall", 0};
    prof_sys_arr[static_cast<uint32_t>(opc_sys::i_ebreak)] = {"ebreak", 0};
    
    prof_csr_arr[static_cast<uint32_t>(opc_csr::i_csrrw)] = {"csrrw", 0};
    prof_csr_arr[static_cast<uint32_t>(opc_csr::i_csrrs)] = {"csrrs", 0};
    prof_csr_arr[static_cast<uint32_t>(opc_csr::i_csrrc)] = {"csrrc", 0};
    prof_csr_arr[static_cast<uint32_t>(opc_csr::i_csrrwi)] = {"csrrwi", 0};
    prof_csr_arr[static_cast<uint32_t>(opc_csr::i_csrrsi)] = {"csrrsi", 0};
    prof_csr_arr[static_cast<uint32_t>(opc_csr::i_csrrci)] = {"csrrci", 0};

    prof_j_arr[static_cast<uint32_t>(opc_j::i_beq)] = {"beq", 0, 0, 0, 0};
    prof_j_arr[static_cast<uint32_t>(opc_j::i_bne)] = {"bne", 0, 0, 0, 0};
    prof_j_arr[static_cast<uint32_t>(opc_j::i_blt)] = {"blt", 0, 0, 0, 0};
    prof_j_arr[static_cast<uint32_t>(opc_j::i_bge)] = {"bge", 0, 0, 0, 0};
    prof_j_arr[static_cast<uint32_t>(opc_j::i_bltu)] = {"bltu", 0, 0, 0, 0};
    prof_j_arr[static_cast<uint32_t>(opc_j::i_bgeu)] = {"bgeu", 0, 0, 0, 0};
    prof_j_arr[static_cast<uint32_t>(opc_j::i_jalr)] = {"jalr", 0, 0, 0, 0};
    prof_j_arr[static_cast<uint32_t>(opc_j::i_jal)] = {"jal", 0, 0, 0, 0};
}

void profiler::log_inst(opc_al opc) {
    if (inst == INST_NOP) {
        prof_ali_arr[static_cast<uint32_t>(opc_al::i_nop)].count++;
    } else {
        auto index = static_cast<uint32_t>(al_type);
        (*prof_al_arr_ptrs[index])[static_cast<uint32_t>(opc)].count++;
    }
}

void profiler::log_inst(opc_mem opc) {
    prof_mem_arr[static_cast<uint32_t>(opc)].count++;
}

void profiler::log_inst(opc_upp opc) {
    prof_upp_arr[static_cast<uint32_t>(opc)].count++;
}

void profiler::log_inst(opc_sys opc) {
    prof_sys_arr[static_cast<uint32_t>(opc)].count++;
}

void profiler::log_inst(opc_csr opc) {
    prof_csr_arr[static_cast<uint32_t>(opc)].count++;
}

void profiler::log_inst(opc_j opc, bool taken, b_dir_t direction) {
    if (taken) {
        prof_j_arr[static_cast<uint32_t>(opc)].count_taken++;
        if (direction == b_dir_t::forward)
            prof_j_arr[static_cast<uint32_t>(opc)].count_taken_fwd++;
    } else {
        prof_j_arr[static_cast<uint32_t>(opc)].count_not_taken++;
        if (direction == b_dir_t::forward)
            prof_j_arr[static_cast<uint32_t>(opc)].count_not_taken_fwd++;
    }
}

void profiler::log_to_file() {
    uint32_t profiled_inst_cnt = 0;
    out_stream.open(log_name + "_inst_profiler.json");
    out_stream << "{\n";
    for (auto &i : prof_alr_arr) {
        if (i.name != "") {
            out_stream << JSON_ENTRY(i.name, i.count) << std::endl;
            profiled_inst_cnt += i.count;
        }
    }
    for (auto &i : prof_ali_arr) {
        if (i.name != "") {
            out_stream << JSON_ENTRY(i.name, i.count) << std::endl;
            profiled_inst_cnt += i.count;
        }
    }
    for (auto &i : prof_mem_arr) {
        out_stream << JSON_ENTRY(i.name, i.count) << std::endl;
        profiled_inst_cnt += i.count;
    }
    for (auto &i : prof_upp_arr) {
        out_stream << JSON_ENTRY(i.name, i.count) << std::endl;
        profiled_inst_cnt += i.count;
    }
    for (auto &i : prof_sys_arr) {
        out_stream << JSON_ENTRY(i.name, i.count) << std::endl;
        profiled_inst_cnt += i.count;
    }
    for (auto &i : prof_csr_arr) {
        out_stream << JSON_ENTRY(i.name, i.count) << std::endl;
        profiled_inst_cnt += i.count;
    }
    
    for (std::size_t i = 0; i < prof_j_arr.size(); ++i) {
        inst_prof_j& e = prof_j_arr[i];
        out_stream << JSON_ENTRY_J(e.name, e.count_taken, e.count_taken_fwd,
                                   e.count_not_taken, e.count_not_taken_fwd);
        out_stream << std::endl;
        profiled_inst_cnt += e.count_taken + e.count_not_taken;
    }

    out_stream << "\"_profiled_instructions\": " << profiled_inst_cnt;
    out_stream << "\n}\n";
    out_stream.close();

    uint32_t executed_pcs = 0;
    out_stream.open(log_name + "_pc-cnt_profiler.json");
    out_stream << "{\n";
    for (size_t i = 0; i < pc_cnt.size(); i++) {
        if (pc_cnt[i] == 0) continue;
        out_stream << JSON_ENTRY(i, pc_cnt[i]) << std::endl;
        executed_pcs += pc_cnt[i];
    }
    out_stream << "\"_executed_pcs\": " << profiled_inst_cnt;
    out_stream << "\n}\n";
    out_stream.close();

    // TODO: add instruction dasm to the profiler as a second entry?
    // executed_pcs = 0;
    // out_stream.open(log_name + "_pc-exec_profiler.json");
    // out_stream << "{\n";
    // for (size_t i = 0; i < pc_exec.size(); i++) {
    //     out_stream << JSON_ENTRY(i, pc_exec[i]) << std::endl;
    //     executed_pcs++;
    // }
    // out_stream << "\"_executed_pcs\": " << executed_pcs;
    // out_stream << "\n}\n";

    #ifndef DPI
    info(inst_cnt, profiled_inst_cnt);
    #endif
}

void profiler::info(uint32_t inst_cnt, uint32_t profiled_inst_cnt){
    std::cout << "Profiler: instructions captured: " 
              << inst_cnt << std::endl;
    std::cout << "Profiler: instructions profiled: "
              << profiled_inst_cnt << std::endl;
}

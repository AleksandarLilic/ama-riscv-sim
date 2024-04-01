#include "profiler.h"

profiler::profiler() {
    inst_cnt = 0;
    
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

void profiler::dump() {
    // TODO: rework, possibly log to file
    uint32_t total_insts = 0;
    std::cout << "Instructions executed: " << inst_cnt << std::endl;
    std::cout << "Instruction breakdown:" << std::endl;
    for (auto &i : prof_alr_arr) {
        if (i.name != "") {
            std::cout << i.name << ": " << i.count << std::endl;
            total_insts += i.count;
        }
    }
    for (auto &i : prof_ali_arr) {
        if (i.name != "") {
            std::cout << i.name << ": " << i.count << std::endl;
            total_insts += i.count;
        }
    }
    for (auto &i : prof_mem_arr) {
        std::cout << i.name << ": " << i.count << std::endl;
        total_insts += i.count;
    }
    for (auto &i : prof_upp_arr) {
        std::cout << i.name << ": " << i.count << std::endl;
        total_insts += i.count;
    }
    for (auto &i : prof_sys_arr) {
        std::cout << i.name << ": " << i.count << std::endl;
        total_insts += i.count;
    }
    for (auto &i : prof_csr_arr) {
        std::cout << i.name << ": " << i.count << std::endl;
        total_insts += i.count;
    }
    
    uint32_t total_taken = 0;
    uint32_t total_taken_fwd = 0;
    uint32_t total_not_taken = 0;
    uint32_t total_not_taken_fwd = 0;
    for (std::size_t i = 0; i < prof_j_arr.size(); ++i) {
        inst_prof_j& e = prof_j_arr[i];
        std::cout << e.name << ": " << e.count_taken + e.count_not_taken 
                  << "; t: " << e.count_taken << " (f/b: " 
                  << e.count_taken_fwd << "/" 
                  << e.count_taken - e.count_taken_fwd << ")";
        
        if ((opc_j)i == opc_j::i_jalr || (opc_j)i == opc_j::i_jal) {
            // jumps always taken
            std::cout << std::endl;
        } else {
            std::cout << "; nt: " << e.count_not_taken << " (f/b: " 
                      << e.count_not_taken_fwd << "/" 
                      << e.count_not_taken - e.count_not_taken_fwd << ")" 
                      << std::endl;
            
            total_taken += e.count_taken;
            total_taken_fwd += e.count_taken_fwd;
            total_not_taken += e.count_not_taken;
            total_not_taken_fwd += e.count_not_taken_fwd;
        }
        total_insts += e.count_taken + e.count_not_taken;
    }

    std::cout << "total branches: " << total_taken + total_not_taken 
              << "; t: " << total_taken << " (f/b: " 
              << total_taken_fwd << "/" 
              << total_taken - total_taken_fwd << ")"
              << "; nt: " << total_not_taken << " (f/b: " 
              << total_not_taken_fwd << "/" 
              << total_not_taken - total_not_taken_fwd << ")" 
              << std::endl;
    
    float predicted = (total_taken - total_taken_fwd) + total_not_taken_fwd;
    float mispredicted = total_taken_fwd + (total_not_taken - total_not_taken_fwd);
    std::cout << "BTFN - p: " << predicted << ", mp: " << mispredicted
              << "; prediction ratio: " << predicted / (predicted + mispredicted)
              << std::endl;
    std::cout << "Instructions profiled: " << total_insts << std::endl;
}

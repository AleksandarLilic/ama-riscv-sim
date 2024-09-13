#include "profiler.h"

profiler::profiler(std::string log_name) {
    inst_cnt = 0;
    this->log_name = log_name;

    prof_alr_arr[static_cast<uint32_t>(opc_al_r::i_add)] = {"add", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al_r::i_sub)] = {"sub", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al_r::i_sll)] = {"sll", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al_r::i_srl)] = {"srl", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al_r::i_sra)] = {"sra", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al_r::i_slt)] = {"slt", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al_r::i_sltu)] = {"sltu", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al_r::i_xor)] = {"xor", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al_r::i_or)] = {"or", 0};
    prof_alr_arr[static_cast<uint32_t>(opc_al_r::i_and)] = {"and", 0};

    prof_alr_mul_arr[static_cast<uint32_t>(opc_al_r_mul::i_mul)] = {"mul", 0};
    prof_alr_mul_arr[static_cast<uint32_t>(opc_al_r_mul::i_mulh)] = {"mulh", 0};
    prof_alr_mul_arr[static_cast<uint32_t>(opc_al_r_mul::i_mulhsu)] = {"mulsu", 0};
    prof_alr_mul_arr[static_cast<uint32_t>(opc_al_r_mul::i_mulhu)] = {"mulu", 0};
    prof_alr_mul_arr[static_cast<uint32_t>(opc_al_r_mul::i_div)] = {"div", 0};
    prof_alr_mul_arr[static_cast<uint32_t>(opc_al_r_mul::i_divu)] = {"divu", 0};
    prof_alr_mul_arr[static_cast<uint32_t>(opc_al_r_mul::i_rem)] = {"rem", 0};
    prof_alr_mul_arr[static_cast<uint32_t>(opc_al_r_mul::i_remu)] = {"remu", 0};

    prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_nop)] = {"nop", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_addi)] = {"addi", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_slli)] = {"slli", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_srli)] = {"srli", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_srai)] = {"srai", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_slti)] = {"slti", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_sltiu)] = {"sltiu", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_xori)] = {"xori", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_ori)] = {"ori", 0};
    prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_andi)] = {"andi", 0};

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

void profiler::log_inst(opc_al_r opc) {
    prof_alr_arr[static_cast<uint32_t>(opc)].count++;
}

void profiler::log_inst(opc_al_i opc) {
    if (inst == INST_NOP) {
        prof_ali_arr[static_cast<uint32_t>(opc_al_i::i_nop)].count++;
    } else {
        prof_ali_arr[static_cast<uint32_t>(opc)].count++;
    }
}

void profiler::log_inst(opc_al_r_mul opc) {
    prof_alr_mul_arr[static_cast<uint32_t>(opc)].count++;
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
    for (auto &i : prof_alr_mul_arr) {
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

    uint32_t min_sp = BASE_ADDR + MEM_SIZE;
    for (const auto& t : trace) {
        if (t.sp != 0 && t.sp < min_sp)
            min_sp = t.sp;
    }
    min_sp = BASE_ADDR + MEM_SIZE - min_sp;

    out_stream << "\"_max_sp_usage\": " << min_sp << ",\n";
    out_stream << "\"_profiled_instructions\": " << profiled_inst_cnt;
    out_stream << "\n}\n";
    out_stream.close();

    out_stream.open(log_name + "_trace.bin", std::ios::binary);
    out_stream.write(reinterpret_cast<char*>(trace.data()),
                     trace.size() * sizeof(trace_entry));
    out_stream.close();

    #ifndef DPI
    info(profiled_inst_cnt, min_sp);
    #endif

    assert(inst_cnt == profiled_inst_cnt &&
           "Profiler: instruction count mismatch");
}

void profiler::info(uint32_t profiled_inst_cnt, uint32_t max_sp){
    std::cout << "Profiler: instructions profiled: "
              << profiled_inst_cnt << std::endl;
    std::cout << "Profiler: max SP usage: "
              << max_sp << std::endl;
}

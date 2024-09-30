#include "profiler.h"

profiler::profiler(std::string log_name) {
    inst_cnt = 0;
    rst_te();
    this->log_name = log_name;

    prof_g_arr[TO_U32(opc_g::i_add)] = {"add", 0};
    prof_g_arr[TO_U32(opc_g::i_sub)] = {"sub", 0};
    prof_g_arr[TO_U32(opc_g::i_sll)] = {"sll", 0};
    prof_g_arr[TO_U32(opc_g::i_srl)] = {"srl", 0};
    prof_g_arr[TO_U32(opc_g::i_sra)] = {"sra", 0};
    prof_g_arr[TO_U32(opc_g::i_slt)] = {"slt", 0};
    prof_g_arr[TO_U32(opc_g::i_sltu)] = {"sltu", 0};
    prof_g_arr[TO_U32(opc_g::i_xor)] = {"xor", 0};
    prof_g_arr[TO_U32(opc_g::i_or)] = {"or", 0};
    prof_g_arr[TO_U32(opc_g::i_and)] = {"and", 0};

    prof_g_arr[TO_U32(opc_g::i_nop)] = {"nop", 0};
    prof_g_arr[TO_U32(opc_g::i_addi)] = {"addi", 0};
    prof_g_arr[TO_U32(opc_g::i_slli)] = {"slli", 0};
    prof_g_arr[TO_U32(opc_g::i_srli)] = {"srli", 0};
    prof_g_arr[TO_U32(opc_g::i_srai)] = {"srai", 0};
    prof_g_arr[TO_U32(opc_g::i_slti)] = {"slti", 0};
    prof_g_arr[TO_U32(opc_g::i_sltiu)] = {"sltiu", 0};
    prof_g_arr[TO_U32(opc_g::i_xori)] = {"xori", 0};
    prof_g_arr[TO_U32(opc_g::i_ori)] = {"ori", 0};
    prof_g_arr[TO_U32(opc_g::i_andi)] = {"andi", 0};
    prof_g_arr[TO_U32(opc_g::i_hint)] = {"hint", 0};

    prof_g_arr[TO_U32(opc_g::i_lb)] = {"lb", 0};
    prof_g_arr[TO_U32(opc_g::i_lh)] = {"lh", 0};
    prof_g_arr[TO_U32(opc_g::i_lw)] = {"lw", 0};
    prof_g_arr[TO_U32(opc_g::i_lbu)] = {"lbu", 0};
    prof_g_arr[TO_U32(opc_g::i_lhu)] = {"lhu", 0};
    prof_g_arr[TO_U32(opc_g::i_sb)] = {"sb", 0};
    prof_g_arr[TO_U32(opc_g::i_sh)] = {"sh", 0};
    prof_g_arr[TO_U32(opc_g::i_sw)] = {"sw", 0};
    prof_g_arr[TO_U32(opc_g::i_fence_i)] = {"fence.i", 0};
    prof_g_arr[TO_U32(opc_g::i_fence)] = {"fence", 0};

    prof_g_arr[TO_U32(opc_g::i_lui)] = {"lui", 0};
    prof_g_arr[TO_U32(opc_g::i_auipc)] = {"auipc", 0};

    prof_g_arr[TO_U32(opc_g::i_ecall)] = {"ecall", 0};
    prof_g_arr[TO_U32(opc_g::i_ebreak)] = {"ebreak", 0};

    prof_g_arr[TO_U32(opc_g::i_csrrw)] = {"csrrw", 0};
    prof_g_arr[TO_U32(opc_g::i_csrrs)] = {"csrrs", 0};
    prof_g_arr[TO_U32(opc_g::i_csrrc)] = {"csrrc", 0};
    prof_g_arr[TO_U32(opc_g::i_csrrwi)] = {"csrrwi", 0};
    prof_g_arr[TO_U32(opc_g::i_csrrsi)] = {"csrrsi", 0};
    prof_g_arr[TO_U32(opc_g::i_csrrci)] = {"csrrci", 0};

    // M extension
    prof_g_arr[TO_U32(opc_g::i_mul)] = {"mul", 0};
    prof_g_arr[TO_U32(opc_g::i_mulh)] = {"mulh", 0};
    prof_g_arr[TO_U32(opc_g::i_mulhsu)] = {"mulsu", 0};
    prof_g_arr[TO_U32(opc_g::i_mulhu)] = {"mulu", 0};
    prof_g_arr[TO_U32(opc_g::i_div)] = {"div", 0};
    prof_g_arr[TO_U32(opc_g::i_divu)] = {"divu", 0};
    prof_g_arr[TO_U32(opc_g::i_rem)] = {"rem", 0};
    prof_g_arr[TO_U32(opc_g::i_remu)] = {"remu", 0};

    // C extension
    prof_g_arr[TO_U32(opc_g::i_c_add)] = {"c.add", 0};
    prof_g_arr[TO_U32(opc_g::i_c_mv)] = {"c.mv", 0};
    prof_g_arr[TO_U32(opc_g::i_c_and)] = {"c.and", 0};
    prof_g_arr[TO_U32(opc_g::i_c_or)] = {"c.or", 0};
    prof_g_arr[TO_U32(opc_g::i_c_xor)] = {"c.xor", 0};
    prof_g_arr[TO_U32(opc_g::i_c_sub)] = {"c.sub", 0};
    prof_g_arr[TO_U32(opc_g::i_c_addi)] = {"c.addi", 0};
    prof_g_arr[TO_U32(opc_g::i_c_addi16sp)] = {"c.addi16sp", 0};
    prof_g_arr[TO_U32(opc_g::i_c_addi4spn)] = {"c.addi4spn", 0};
    prof_g_arr[TO_U32(opc_g::i_c_andi)] = {"c.andi", 0};
    prof_g_arr[TO_U32(opc_g::i_c_srli)] = {"c.srli", 0};
    prof_g_arr[TO_U32(opc_g::i_c_slli)] = {"c.slli", 0};
    prof_g_arr[TO_U32(opc_g::i_c_srai)] = {"c.srai", 0};
    prof_g_arr[TO_U32(opc_g::i_c_nop)] = {"c.nop", 0};
    prof_g_arr[TO_U32(opc_g::i_c_lwsp)] = {"c.lwsp", 0};
    prof_g_arr[TO_U32(opc_g::i_c_swsp)] = {"c.swsp", 0};
    prof_g_arr[TO_U32(opc_g::i_c_lw)] = {"c.lw", 0};
    prof_g_arr[TO_U32(opc_g::i_c_sw)] = {"c.sw", 0};
    prof_g_arr[TO_U32(opc_g::i_c_li)] = {"c.li", 0};
    prof_g_arr[TO_U32(opc_g::i_c_lui)] = {"c.lui", 0};
    prof_g_arr[TO_U32(opc_g::i_c_ebreak)] = {"c.ebreak", 0};

    // Control transfer
    prof_j_arr[TO_U32(opc_j::i_beq)] = {"beq", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_bne)] = {"bne", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_blt)] = {"blt", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_bge)] = {"bge", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_bltu)] = {"bltu", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_bgeu)] = {"bgeu", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_jalr)] = {"jalr", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_jal)] = {"jal", 0, 0, 0, 0};
    // C extension
    prof_j_arr[TO_U32(opc_j::i_c_j)] = {"c.j", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_c_jal)] = {"c.jal", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_c_jr)] = {"c.jr", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_c_jalr)] = {"c.jalr", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_c_beqz)] = {"c.beqz", 0, 0, 0, 0};
    prof_j_arr[TO_U32(opc_j::i_c_bnez)] = {"c.bnez", 0, 0, 0, 0};
}

void profiler::new_inst(uint32_t inst) {
    if (active) {
        this->inst = inst;
        inst_cnt++;
    }
}

void profiler::log_inst(opc_g opc) {
    if (active) {
        if (inst == INST_NOP)
            prof_g_arr[TO_U32(opc_g::i_nop)].count++;
        else if ((inst & 0xFFFF) == INST_C_NOP) // 16-bit inst
            prof_g_arr[TO_U32(opc_g::i_c_nop)].count++;
        else if (inst == INST_HINT_LOG_START || inst == INST_HINT_LOG_END)
            prof_g_arr[TO_U32(opc_g::i_hint)].count++;
        else
            prof_g_arr[TO_U32(opc)].count++;
    }
}

void profiler::log_inst(opc_j opc, bool taken, b_dir_t direction) {
    if (active) {
        if (taken) {
            prof_j_arr[TO_U32(opc)].count_taken++;
            if (direction == b_dir_t::forward)
                prof_j_arr[TO_U32(opc)].count_taken_fwd++;
        } else {
            prof_j_arr[TO_U32(opc)].count_not_taken++;
            if (direction == b_dir_t::forward)
                prof_j_arr[TO_U32(opc)].count_not_taken_fwd++;
        }
    }
}

void profiler::log() {
    if (active) {
        trace.push_back(te);
        rst_te();
    }
}

void profiler::log_reg_use(reg_use_t reg_use, uint8_t reg) {
    prof_reg_hist[reg][TO_U8(reg_use)]++;
}

void profiler::log_to_file() {
    uint32_t profiled_inst_cnt = 0;
    ofs.open(log_name + "_inst_profiler.json");
    ofs << "{\n";
    for (auto &i : prof_g_arr) {
        if (i.name != "") {
            ofs << JSON_ENTRY(i.name, i.count) << std::endl;
            profiled_inst_cnt += i.count;
        }
    }

    for (std::size_t i = 0; i < prof_j_arr.size(); ++i) {
        inst_prof_j& e = prof_j_arr[i];
        ofs << JSON_ENTRY_J(e.name, e.count_taken, e.count_taken_fwd,
                                   e.count_not_taken, e.count_not_taken_fwd);
        ofs << std::endl;
        profiled_inst_cnt += e.count_taken + e.count_not_taken;
    }

    uint32_t min_sp = BASE_ADDR + MEM_SIZE;
    for (const auto& t : trace) {
        if (t.sp != 0 && t.sp < min_sp)
            min_sp = t.sp;
    }
    min_sp = BASE_ADDR + MEM_SIZE - min_sp;

    ofs << "\"_max_sp_usage\": " << min_sp << ",\n";
    ofs << "\"_profiled_instructions\": " << profiled_inst_cnt;
    ofs << "\n}\n";
    ofs.close();

    ofs.open(log_name + "_trace.bin", std::ios::binary);
    ofs.write(reinterpret_cast<char*>(trace.data()),
              trace.size() * sizeof(trace_entry));
    ofs.close();

    #ifndef DPI
    info(profiled_inst_cnt, min_sp);
    #endif

    ofs.open(log_name + "_reg_hist.bin", std::ios::binary);
    ofs.write(reinterpret_cast<char*>(prof_reg_hist.data()),
             prof_reg_hist.size() * prof_reg_hist[0].size() * sizeof(uint32_t));
    ofs.close();

    assert(inst_cnt == profiled_inst_cnt &&
           "Profiler: instruction count mismatch");
}

void profiler::info(uint32_t profiled_inst_cnt, uint32_t max_sp){
    std::cout << "Profiler: instructions profiled: "
              << profiled_inst_cnt << std::endl;
    std::cout << "Profiler: max SP usage: "
              << max_sp << std::endl;
}

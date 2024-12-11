#include "profiler.h"

profiler::profiler(std::string log_path) {
    inst_cnt_exec = 0;
    rst_te();
    this->log_path = log_path;

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

    prof_g_arr[TO_U32(opc_g::i_dot16)] = {"dot16", 0};
    prof_g_arr[TO_U32(opc_g::i_dot8)] = {"dot8", 0};
    prof_g_arr[TO_U32(opc_g::i_dot4)] = {"dot4", 0};

    prof_g_arr[TO_U32(opc_g::i_unpk16)] = {"unpk16", 0};
    prof_g_arr[TO_U32(opc_g::i_unpk16u)] = {"unpk16u", 0};
    prof_g_arr[TO_U32(opc_g::i_unpk8)] = {"unpk8", 0};
    prof_g_arr[TO_U32(opc_g::i_unpk8u)] = {"unpk8u", 0};
    prof_g_arr[TO_U32(opc_g::i_unpk4)] = {"unpk4", 0};
    prof_g_arr[TO_U32(opc_g::i_unpk4u)] = {"unpk4u", 0};
    prof_g_arr[TO_U32(opc_g::i_unpk2)] = {"unpk2", 0};
    prof_g_arr[TO_U32(opc_g::i_unpk2u)] = {"unpk2u", 0};

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

    prof_g_arr[TO_U32(opc_g::i_scp_lcl)] = {"scp.lcl", 0};
    prof_g_arr[TO_U32(opc_g::i_scp_rel)] = {"scp.rel", 0};

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
        inst_cnt_exec++;
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
    cnt_t cnt;
    ofs.open(log_path + "inst_profiler.json");
    ofs << "{\n";
    for (const auto &i : prof_g_arr) {
        if (i.name != "") {
            ofs << PROF_JSON_ENTRY(i.name, i.count) << std::endl;
            cnt.inst += i.count;
        }
    }

    for (const auto &e : prof_j_arr) {
        ofs << PROF_JSON_ENTRY_J(e.name, e.count_taken, e.count_taken_fwd,
                                 e.count_not_taken, e.count_not_taken_fwd);
        ofs << std::endl;
        cnt.inst += e.count_taken + e.count_not_taken;
    }

    uint32_t min_sp = BASE_ADDR + MEM_SIZE;
    for (const auto& t : trace) {
        if (t.sp != 0 && t.sp < min_sp)
            min_sp = t.sp;
    }
    min_sp = BASE_ADDR + MEM_SIZE - min_sp;

    ofs << "\"_max_sp_usage\": " << min_sp << ",\n";
    ofs << "\"_profiled_instructions\": " << cnt.inst;
    ofs << "\n}\n";
    ofs.close();

    ofs.open(log_path + "trace.bin", std::ios::binary);
    ofs.write(reinterpret_cast<char*>(trace.data()),
              trace.size() * sizeof(trace_entry));
    ofs.close();

    ofs.open(log_path + "reg_hist.bin", std::ios::binary);
    ofs.write(reinterpret_cast<char*>(prof_reg_hist.data()),
             prof_reg_hist.size() * prof_reg_hist[0].size() * sizeof(uint32_t));
    ofs.close();

    // compressed inst cnt
    uint32_t c_cnt = 0;
    for (auto &c: c_opcs_alu) c_cnt += prof_g_arr[TO_U32(c)].count;
    for (auto &c: c_opcs_j) {
        c_cnt += prof_j_arr[TO_U32(c)].count_taken +
                 prof_j_arr[TO_U32(c)].count_not_taken;
    }
    float_t c_perc = 100.0 * c_cnt / cnt.inst;

    // per type breakdown
    for (auto &b: branch_opcs) {
        cnt.branch += prof_j_arr[TO_U32(b)].count_taken +
                      prof_j_arr[TO_U32(b)].count_not_taken;
    }
    for (auto &j: jump_opcs) cnt.jump += prof_j_arr[TO_U32(j)].count_taken;
    for (auto &l: load_opcs) cnt.load += prof_g_arr[TO_U32(l)].count;
    for (auto &s: store_opcs) cnt.store += prof_g_arr[TO_U32(s)].count;
    for (auto &a: alu_opcs) cnt.al += prof_g_arr[TO_U32(a)].count;
    for (auto &m: mul_opcs) cnt.mul += prof_g_arr[TO_U32(m)].count;
    for (auto &d: div_opcs) cnt.div += prof_g_arr[TO_U32(d)].count;
    for (auto &f: dot_opcs) cnt.dot += prof_g_arr[TO_U32(f)].count;
    for (auto &u: unpk_opcs) cnt.unpk += prof_g_arr[TO_U32(u)].count;
    for (auto &s: scp_opcs) cnt.scp += prof_g_arr[TO_U32(s)].count;
    cnt.find_mem();
    cnt.find_rest();

    perc_t perc;
    perc.branch = cnt.get_perc(cnt.branch);
    perc.jump = cnt.get_perc(cnt.jump);
    perc.load = cnt.get_perc(cnt.load);
    perc.store = cnt.get_perc(cnt.store);
    perc.mem = cnt.get_perc(cnt.mem);
    perc.al = cnt.get_perc(cnt.al);
    perc.mul = cnt.get_perc(cnt.mul);
    perc.div = cnt.get_perc(cnt.div);
    perc.dot = cnt.get_perc(cnt.dot);
    perc.unpk = cnt.get_perc(cnt.unpk);
    perc.scp = cnt.get_perc(cnt.scp);
    perc.rest = cnt.get_perc(cnt.rest);

    std::cout << std::fixed << std::setprecision(2)
              << "Profiler: Inst: " << cnt.inst
              << " - 32/16-bit: " << cnt.inst - c_cnt << "/" << c_cnt
              << "(" << 100.0 - c_perc << "%/" << c_perc << "%)"
              << std::endl;

    std::cout << INDENT << "Control: B: " << cnt.branch
              << "(" << perc.branch << "%)"
              << ", J: " << cnt.jump
              << "(" << perc.jump << "%)" << std::endl;

    std::cout << INDENT << "Memory: MEM: " << cnt.mem
              << "(" << perc.mem
              << "%) - L/S: " << cnt.load
              << "/" << cnt.store
              << "(" << perc.load
              << "%/" << perc.store << "%)" << std::endl;

    std::cout << INDENT << "Compute: A&L: " << cnt.al
              << "(" << perc.al << "%)"
              << ", MUL: " << cnt.mul
              << "(" << perc.mul << "%)"
              << ", DIV: " << cnt.div
              << "(" << perc.div << "%)" << std::endl;

    std::cout << INDENT << "SIMD: DOT: " << cnt.dot
              << "(" << perc.dot << "%)"
              << ", UNPK: " << cnt.unpk
              << "(" << perc.unpk << "%)" << std::endl;

    std::cout << INDENT << "Hints: SCP: " << cnt.scp
              << "(" << perc.scp << "%)" << std::endl;

    std::cout << INDENT << "Rest: " << cnt.rest
              << "(" << perc.rest << "%)" << std::endl;

    std::cout << "Profiler: max SP usage: " << min_sp << std::endl;

    // only expected to fail if core has instruction which is
    // not supported by the profiler - should be fixed in code before release
    // not expected to fail in normal operation
    assert(inst_cnt_exec == cnt.inst && "Profiler: instruction count mismatch");
}

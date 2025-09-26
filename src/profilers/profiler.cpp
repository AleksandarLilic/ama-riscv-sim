#include "profiler.h"

profiler::profiler(std::string out_dir, profiler_source_t prof_src) {
    inst = 0;
    inst_cnt_prof = 0;
    trace.reserve(1<<14); // reserve 16K entries to start with
    rst_te();
    this->out_dir = out_dir;
    this->prof_src = prof_src;

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
    prof_g_arr[TO_U32(opc_g::i_mret)] = {"mret", 0};
    prof_g_arr[TO_U32(opc_g::i_wfi)] = {"wfi", 0};

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

    // Custom instructions
    prof_g_arr[TO_U32(opc_g::i_add16)] = {"add16", 0};
    prof_g_arr[TO_U32(opc_g::i_add8)] = {"add8", 0};
    prof_g_arr[TO_U32(opc_g::i_sub16)] = {"sub16", 0};
    prof_g_arr[TO_U32(opc_g::i_sub8)] = {"sub8", 0};
    prof_g_arr[TO_U32(opc_g::i_mul16)] = {"mul16", 0};
    prof_g_arr[TO_U32(opc_g::i_mul16u)] = {"mul16u", 0};
    prof_g_arr[TO_U32(opc_g::i_mul8)] = {"mul8", 0};
    prof_g_arr[TO_U32(opc_g::i_mul8u)] = {"mul8u", 0};
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

    prof_g_arr[TO_U32(opc_g::i_scp_lcl)] = {"scp.lcl", 0};
    prof_g_arr[TO_U32(opc_g::i_scp_rel)] = {"scp.rel", 0};

    // Zicsr extension
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

    // Zbb extension
    prof_g_arr[TO_U32(opc_g::i_max)] = {"max", 0};
    prof_g_arr[TO_U32(opc_g::i_maxu)] = {"maxu", 0};
    prof_g_arr[TO_U32(opc_g::i_min)] = {"min", 0};
    prof_g_arr[TO_U32(opc_g::i_minu)] = {"minu", 0};

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

void profiler::add_te() {
    if (active && trace_en) {
        trace.push_back(te);
        rst_te(); // in case next instruction doesn't update all fields
    }
}

void profiler::log_inst(opc_g opc, uint64_t inc) {
    if (active) {
        inst_cnt_prof++;
        if (inst == INST_NOP) {
            prof_g_arr[TO_U32(opc_g::i_nop)].count += inc;
        } else if ((inst & 0xFFFF) == INST_C_NOP) {
            prof_g_arr[TO_U32(opc_g::i_c_nop)].count += inc;
        } else if (inst == INST_HINT_LOG_START || inst == INST_HINT_LOG_END) {
            prof_g_arr[TO_U32(opc_g::i_hint)].count += inc;
        } else {
            prof_g_arr[TO_U32(opc)].count += inc;
        }
    }
}

void profiler::log_inst(opc_j opc, bool taken, b_dir_t b_dir, uint64_t inc) {
    if (active) {
        inst_cnt_prof++;
        if (taken) {
            prof_j_arr[TO_U32(opc)].count_taken += inc;
            if (b_dir == b_dir_t::forward) {
                prof_j_arr[TO_U32(opc)].count_taken_fwd += inc;
            }
        } else {
            prof_j_arr[TO_U32(opc)].count_not_taken += inc;
            if (b_dir == b_dir_t::forward) {
                prof_j_arr[TO_U32(opc)].count_not_taken_fwd += inc;
            }
        }
    }
}

void profiler::log_reg_use(reg_use_t reg_use, uint8_t reg) {
    if (active && rf_usage) prof_rf_usage[reg][TO_U8(reg_use)]++;
}

void profiler::track_sp(const uint32_t sp) {
    if (sp != 0 && sp < min_sp) min_sp = sp;
}

void profiler::log_to_file_and_print() {
    cnt_t cnt;
    std::string pt = "";
    if (prof_src == profiler_source_t::clock) pt = "_clk";

    ofs.open(out_dir + "inst_profiler" + pt + ".json");
    ofs << "{\n";
    for (const auto &i : prof_g_arr) {
        if (i.name != "") {
            ofs << PROF_JSON_ENTRY(i.name, i.count) << "\n";
            cnt.tot += i.count;
        }
    }

    for (const auto &e : prof_j_arr) {
        ofs << PROF_JSON_ENTRY_J(e.name, e.count_taken, e.count_taken_fwd,
                                 e.count_not_taken, e.count_not_taken_fwd);
        ofs << "\n";
        cnt.tot += e.count_taken + e.count_not_taken;
    }

    min_sp = BASE_ADDR + MEM_SIZE - min_sp; // remove offset
    ofs << "\"_max_sp_usage\": " << min_sp << ",\n";
    if (prof_src == profiler_source_t::clock) {
        ofs << "\"_profiled_cycles\": " << cnt.tot;
    } else {
        ofs << "\"_profiled_instructions\": " << cnt.tot;
    }
    ofs << "\n}\n";
    ofs.close();

    if (trace_en) {
        ofs.open(out_dir + "trace" + pt + ".bin", std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(trace.data()),
                trace.size() * sizeof(trace_entry));
        ofs.close();
    }

    if (rf_usage) {
        ofs.open(out_dir + "rf_usage" +  pt + ".bin", std::ios::binary);
        ofs.write(
            reinterpret_cast<const char*>(prof_rf_usage.data()),
            prof_rf_usage.size() * prof_rf_usage[0].size() * sizeof(uint32_t));
        ofs.close();
    }

    // compressed inst cnt
    uint32_t comp_cnt = 0;
    for (auto &c: comp_opcs_alu) comp_cnt += prof_g_arr[TO_U32(c)].count;
    for (auto &c: comp_opcs_j) {
        comp_cnt += prof_j_arr[TO_U32(c)].count_taken +
                    prof_j_arr[TO_U32(c)].count_not_taken;
    }
    float_t comp_perc = cnt.get_perc(comp_cnt);

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
    for (auto &f: dot_c_opcs) cnt.dot_c += prof_g_arr[TO_U32(f)].count;
    for (auto &a: al_c_opcs) cnt.al_c += prof_g_arr[TO_U32(a)].count;
    for (auto &m: mul_c_opcs) cnt.mul_c += prof_g_arr[TO_U32(m)].count;
    for (auto &z: zbb_opcs) cnt.zbb += prof_g_arr[TO_U32(z)].count;
    for (auto &u: unpk_c_opcs) cnt.unpk_c += prof_g_arr[TO_U32(u)].count;
    for (auto &s: scp_c_opcs) cnt.scp_c += prof_g_arr[TO_U32(s)].count;
    cnt.nop = prof_g_arr[TO_U32(opc_g::i_nop)].count;
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
    perc.dot_c = cnt.get_perc(cnt.dot_c);
    perc.al_c = cnt.get_perc(cnt.al_c);
    perc.mul_c = cnt.get_perc(cnt.mul_c);
    perc.zbb = cnt.get_perc(cnt.zbb);
    perc.unpk_c = cnt.get_perc(cnt.unpk_c);
    perc.scp_c = cnt.get_perc(cnt.scp_c);
    perc.rest = cnt.get_perc(cnt.rest);
    perc.nop = cnt.get_perc(cnt.nop);

    std::cout << inst_cnt_prof << " instructions profiled\n";
    #ifdef DPI
    return;
    #endif

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Profiler: "
              << "Inst: " << cnt.tot
              << " - 32/16-bit: " << cnt.tot - comp_cnt << "/" << comp_cnt
              << "(" << 100.0 - comp_perc << "%/" << comp_perc << "%)"
              << "\n";

    std::cout << INDENT << "Control:"
              << " B: " << cnt.branch << "(" << perc.branch << "%),"
              << " J: " << cnt.jump << "(" << perc.jump << "%),"
              << " NOP: " << cnt.nop << "(" << perc.nop << "%)"
              << "\n";

    std::cout << INDENT << "Memory:"
              << " MEM: " << cnt.mem << "(" << perc.mem << "%)"
              << " - L/S: " << cnt.load << "/" << cnt.store
              << "(" << perc.load << "%/" << perc.store << "%)"
              << "\n";

    std::cout << INDENT << "Compute: "
              << " A&L: " << cnt.al << "(" << perc.al << "%),"
              << " MUL: " << cnt.mul << "(" << perc.mul << "%),"
              << " DIV: " << cnt.div << "(" << perc.div << "%)"
              << "\n";

    std::cout << INDENT << "Bitmanip:"
              << " Zbb: " << cnt.zbb << "(" << perc.zbb << "%)"
              << "\n";

    std::cout << INDENT << "SIMD:"
              << " A&L: " << cnt.al_c << "(" << perc.al_c << "%),"
              << " MUL: " << cnt.mul_c << "(" << perc.mul_c << "%),"
              << " DOT: " << cnt.dot_c << "(" << perc.dot_c << "%),"
              << " UNPK: " << cnt.unpk_c << "(" << perc.unpk_c << "%)"
              << "\n";

    std::cout << INDENT << "Hints:"
              << " SCP: " << cnt.scp_c << "(" << perc.scp_c << "%)"
              << "\n";

    std::cout << INDENT
              << "Rest: " << cnt.rest << "(" << perc.rest << "%)"
              << "\n";

    if (prof_src == profiler_source_t::inst) {
        float_t sparsity = sparsity_cnt.get_perc();
        std::cout << "Profiler Sparsity: total: " << sparsity_cnt.total
                << ", sparse: " << sparsity_cnt.sparse
                << "(" << sparsity << "%)" << "\n";
    }

    uint64_t sa_cnt = stack_access.total();
    uint64_t sa_cnt_load = stack_access.get_load();
    uint64_t sa_cnt_store = stack_access.get_store();
    float_t sa_perc = 0.0;
    float_t sa_perc_load = 0.0;
    float_t sa_perc_store = 0.0;
    if (cnt.mem) sa_perc = 100.0 * sa_cnt / cnt.mem;
    if (cnt.load) sa_perc_load = 100.0 * sa_cnt_load / cnt.mem;
    if (cnt.store) sa_perc_store = 100.0 * sa_cnt_store / cnt.mem;
    std::cout << "Profiler Stack: peak usage: " << min_sp << " B, Accesses: "
              << sa_cnt << "(" << sa_perc << "%) - L/S: "
              << sa_cnt_load << "/" << sa_cnt_store
              << "(" << sa_perc_load << "%/" << sa_perc_store << "%)"
              << "\n";

    if (prof_src == profiler_source_t::clock) return;
    // only expected to fail if core has instruction which is not supported
    // by the profiler - should be addressed when adding new instructions
    assert(inst_cnt_prof == cnt.tot && "Profiler: instruction count mismatch");
}

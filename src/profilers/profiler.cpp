#include "profiler.h"

profiler::profiler(std::string out_dir, profiler_source_t prof_src) {
    inst = 0;
    inst_cnt_prof = 0;
    trace.reserve(1<<14); // reserve 16K entries to start with
    te.rst();
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
    prof_g_arr[TO_U32(opc_g::i_wmul16)] = {"wmul16", 0};
    prof_g_arr[TO_U32(opc_g::i_wmul16u)] = {"wmul16u", 0};
    prof_g_arr[TO_U32(opc_g::i_wmul8)] = {"wmul8", 0};
    prof_g_arr[TO_U32(opc_g::i_wmul8u)] = {"wmul8u", 0};
    prof_g_arr[TO_U32(opc_g::i_dot16)] = {"dot16", 0};
    prof_g_arr[TO_U32(opc_g::i_dot8)] = {"dot8", 0};
    prof_g_arr[TO_U32(opc_g::i_dot4)] = {"dot4", 0};

    prof_g_arr[TO_U32(opc_g::i_widen16)] = {"widen16", 0};
    prof_g_arr[TO_U32(opc_g::i_widen16u)] = {"widen16u", 0};
    prof_g_arr[TO_U32(opc_g::i_widen8)] = {"widen8", 0};
    prof_g_arr[TO_U32(opc_g::i_widen8u)] = {"widen8u", 0};
    prof_g_arr[TO_U32(opc_g::i_widen4)] = {"widen4", 0};
    prof_g_arr[TO_U32(opc_g::i_widen4u)] = {"widen4u", 0};
    prof_g_arr[TO_U32(opc_g::i_widen2)] = {"widen2", 0};
    prof_g_arr[TO_U32(opc_g::i_widen2u)] = {"widen2u", 0};

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

    // ctrl
    prof_b_arr[TO_U32(opc_b::i_beq)] = {"beq", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_bne)] = {"bne", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_blt)] = {"blt", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_bge)] = {"bge", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_bltu)] = {"bltu", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_bgeu)] = {"bgeu", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_jalr)] = {"jalr", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_jal)] = {"jal", 0, 0, 0, 0};

    // C extension ctrl
    prof_b_arr[TO_U32(opc_b::i_c_j)] = {"c.j", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_c_jal)] = {"c.jal", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_c_jr)] = {"c.jr", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_c_jalr)] = {"c.jalr", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_c_beqz)] = {"c.beqz", 0, 0, 0, 0};
    prof_b_arr[TO_U32(opc_b::i_c_bnez)] = {"c.bnez", 0, 0, 0, 0};
}

void profiler::add_te() {
    if (active && trace_en) {
        trace.push_back(te);
        #ifndef DPI
        // in case next instruction doesn't update all fields
        // DPI te is copied from cosim, no point in resetting
        te.rst();
        #endif
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

void profiler::log_inst(opc_b opc, bool taken, b_dir_t b_dir, uint64_t inc) {
    if (active) {
        inst_cnt_prof++;
        if (taken) {
            prof_b_arr[TO_U32(opc)].count_taken += inc;
            if (b_dir == b_dir_t::forward) {
                prof_b_arr[TO_U32(opc)].count_taken_fwd += inc;
            }
        } else {
            prof_b_arr[TO_U32(opc)].count_not_taken += inc;
            if (b_dir == b_dir_t::forward) {
                prof_b_arr[TO_U32(opc)].count_not_taken_fwd += inc;
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

void profiler::log_to_file_and_print(bool silent) {
    cnt_t cnt;
    std::string pt = "";
    if (prof_src == profiler_source_t::clock) pt = "_clk";

    ofs.open(out_dir + "inst_profile" + pt + ".json");
    ofs << "{\n";
    for (const auto &i : prof_g_arr) {
        if ((i.name != "")
            #ifndef RV32C
            && !(i.name.rfind("c.", 0) == 0)
            #endif
        ) {
            ofs << PROF_JSON_ENTRY(i.name, i.count) << "\n";
            cnt.tot += i.count;
        }
    }

    for (const auto &e : prof_b_arr) {
        // .starts_with() on c++20
        if ((e.name != "")
            #ifndef RV32C
            && !(e.name.rfind("c.", 0) == 0)
            #endif
        ) {
            ofs << PROF_JSON_ENTRY_B(
                e.name, e.count_taken, e.count_taken_fwd,
                e.count_not_taken, e.count_not_taken_fwd
            );
            ofs << "\n";
            cnt.tot += e.count_taken + e.count_not_taken;
        }
    }

    min_sp = BASE_ADDR + MEM_SIZE - min_sp; // remove offset
    ofs << INDENT << "\"_max_sp_usage\": " << min_sp << ",\n" << INDENT;
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
            (prof_rf_usage.size() * // num of regs
             prof_rf_usage[0].size() * // num of options for each reg
             sizeof(prof_rf_usage[0][0])) // counter width
        );
        ofs.close();
    }

    #ifdef RV32C
    // compressed inst cnt
    uint32_t comp_cnt = 0;
    for (auto &c: comp_opcs_alu) comp_cnt += prof_g_arr[TO_U32(c)].count;
    for (auto &c: comp_opcs_b) {
        comp_cnt += prof_b_arr[TO_U32(c)].count_taken +
                    prof_b_arr[TO_U32(c)].count_not_taken;
    }
    float_t comp_perc = cnt.get_perc(comp_cnt);
    #endif

    // per type breakdown
    for (auto &b: branch_opcs) {
        cnt.branch += prof_b_arr[TO_U32(b)].count_taken +
                      prof_b_arr[TO_U32(b)].count_not_taken;
    }
    for (auto &j: jal_opcs) cnt.jal += prof_b_arr[TO_U32(j)].count_taken;
    for (auto &j: jalr_opcs) cnt.jalr += prof_b_arr[TO_U32(j)].count_taken;
    for (auto &l: load_opcs) cnt.load += prof_g_arr[TO_U32(l)].count;
    for (auto &s: store_opcs) cnt.store += prof_g_arr[TO_U32(s)].count;
    for (auto &a: alu_opcs) cnt.alu += prof_g_arr[TO_U32(a)].count;
    for (auto &m: mul_opcs) cnt.mul += prof_g_arr[TO_U32(m)].count;
    for (auto &d: div_opcs) cnt.div += prof_g_arr[TO_U32(d)].count;
    for (auto &f: dot_c_opcs) cnt.dot_c += prof_g_arr[TO_U32(f)].count;
    for (auto &a: alu_c_opcs) cnt.alu_c += prof_g_arr[TO_U32(a)].count;
    for (auto &m: mul_c_opcs) cnt.mul_c += prof_g_arr[TO_U32(m)].count;
    for (auto &z: zbb_opcs) cnt.zbb += prof_g_arr[TO_U32(z)].count;
    for (auto &u: widen_c_opcs) cnt.widen_c += prof_g_arr[TO_U32(u)].count;
    for (auto &s: scp_c_opcs) cnt.scp_c += prof_g_arr[TO_U32(s)].count;
    cnt.nop = prof_g_arr[TO_U32(opc_g::i_nop)].count;
    cnt.find_mem();
    cnt.find_rest();

    perc_t perc;
    perc.branch = cnt.get_perc(cnt.branch);
    perc.jal = cnt.get_perc(cnt.jal);
    perc.jalr = cnt.get_perc(cnt.jalr);
    perc.load = cnt.get_perc(cnt.load);
    perc.store = cnt.get_perc(cnt.store);
    perc.mem = cnt.get_perc(cnt.mem);
    perc.alu = cnt.get_perc(cnt.alu);
    perc.mul = cnt.get_perc(cnt.mul);
    perc.div = cnt.get_perc(cnt.div);
    perc.dot_c = cnt.get_perc(cnt.dot_c);
    perc.alu_c = cnt.get_perc(cnt.alu_c);
    perc.mul_c = cnt.get_perc(cnt.mul_c);
    perc.zbb = cnt.get_perc(cnt.zbb);
    perc.widen_c = cnt.get_perc(cnt.widen_c);
    perc.scp_c = cnt.get_perc(cnt.scp_c);
    perc.rest = cnt.get_perc(cnt.rest);
    perc.nop = cnt.get_perc(cnt.nop);

    #ifdef DPI
    return;
    #endif

    if (silent) return;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Profiler - Inst:\n"
              << INDENT << "All: " << cnt.tot
              #ifdef RV32C
              << " - 32/16-bit: " << (cnt.tot - comp_cnt) << "/" << comp_cnt
              << "(" << (100.0 - comp_perc) << "%/" << comp_perc << "%)"
              #endif
              << "\n";

    std::cout << INDENT << "Control:"
              << " B: " << cnt.branch << "(" << perc.branch << "%),"
              << " JAL: " << cnt.jal << "(" << perc.jal << "%),"
              << " JALR: " << cnt.jalr << "(" << perc.jalr << "%)"
              << "\n";

    std::cout << INDENT << "Memory:"
              << " MEM: " << cnt.mem << "(" << perc.mem << "%)"
              << " - L/S: " << cnt.load << "/" << cnt.store
              << "(" << perc.load << "%/" << perc.store << "%)"
              << "\n";

    std::cout << INDENT << "Compute:"
              << " ALU: " << cnt.alu << "(" << perc.alu << "%),"
              << " MUL: " << cnt.mul << "(" << perc.mul << "%),"
              << " DIV: " << cnt.div << "(" << perc.div << "%)"
              << "\n";

    std::cout << INDENT << "Bitmanip:"
              << " Zbb: " << cnt.zbb << "(" << perc.zbb << "%)"
              << "\n";

    std::cout << INDENT << "SIMD:"
              << " ALU: " << cnt.alu_c << "(" << perc.alu_c << "%),"
              << " MUL: " << cnt.mul_c << "(" << perc.mul_c << "%),"
              << " DOT: " << cnt.dot_c << "(" << perc.dot_c << "%),"
              << " WIDEN: " << cnt.widen_c << "(" << perc.widen_c << "%)"
              << "\n";

    std::cout << INDENT << "Hint:"
              << " SCP: " << cnt.scp_c << "(" << perc.scp_c << "%)"
              << "\n";

    std::cout << INDENT << "Other:"
              << " NOP: " << cnt.nop << "(" << perc.nop << "%),"
              << " Misc: " << cnt.rest << "(" << perc.rest << "%)"
              << "\n";

    std::cout << "Profiler - Sparsity:\n";
    for (size_t i = 0; i < TO_U32(sparsity_t::_count); i++) {
        sparsity_cnt_t* ptr = &sparsity_cnt[i];
        const char* n = sparsity_cnt_names[i];
        std::cout << INDENT << n << ": " << ptr->total << "/" << ptr->sparse
                  << "(" << TO_F32(ptr->get_perc()) << "%)" << "\n";

    }

    uint64_t sa_cnt = stack_access.total();
    uint64_t sa_cnt_load = stack_access.get_load();
    uint64_t sa_cnt_store = stack_access.get_store();
    float_t sa_perc = 0.0;
    float_t sa_perc_load = 0.0;
    float_t sa_perc_store = 0.0;
    if (cnt.mem) sa_perc = (100.0 * sa_cnt / cnt.mem);
    if (cnt.load) sa_perc_load = (100.0 * sa_cnt_load / cnt.mem);
    if (cnt.store) sa_perc_store = (100.0 * sa_cnt_store / cnt.mem);
    std::cout << "Profiler - Stack:\n";
    std::cout << INDENT << "Peak usage: " << min_sp << " B\n"
              << INDENT << "Accesses: "
              << sa_cnt << "(" << sa_perc << "%) - L/S: "
              << sa_cnt_load << "/" << sa_cnt_store
              << "(" << sa_perc_load << "%/" << sa_perc_store << "%)"
              << "\n";

    if (prof_src == profiler_source_t::clock) return;
    // only expected to fail if core has instruction which is not supported
    // by the profiler - should be addressed when adding new instructions
    assert(inst_cnt_prof == cnt.tot && "Profiler: instruction count mismatch");
}

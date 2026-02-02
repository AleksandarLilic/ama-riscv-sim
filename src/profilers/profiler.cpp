#include "profiler.h"

profiler::profiler(std::string out_dir, profiler_source_t prof_src) {
    inst = 0;
    inst_cnt_prof = 0;
    trace.reserve(1<<14); // reserve 16K entries to start with
    te.rst();
    this->out_dir = out_dir;
    this->prof_src = prof_src;

    #define P_INIT(s) \
        prof_g_arr[TO_U32(opc_g::i_##s)] = {#s, 0}

    #define P_INIT_D(s1, s2) \
        prof_g_arr[TO_U32(opc_g::i_##s1##_##s2)] = {#s1"."#s2, 0}

    P_INIT(add);
    P_INIT(sub);
    P_INIT(sll);
    P_INIT(srl);
    P_INIT(sra);
    P_INIT(slt);
    P_INIT(sltu);
    P_INIT(xor);
    P_INIT(or);
    P_INIT(and);

    P_INIT(nop);
    P_INIT(addi);
    P_INIT(slli);
    P_INIT(srli);
    P_INIT(srai);
    P_INIT(slti);
    P_INIT(sltiu);
    P_INIT(xori);
    P_INIT(ori);
    P_INIT(andi);
    P_INIT(hint);
    P_INIT(mret);
    P_INIT(wfi);

    P_INIT(lb);
    P_INIT(lh);
    P_INIT(lw);
    P_INIT(lbu);
    P_INIT(lhu);
    P_INIT(sb);
    P_INIT(sh);
    P_INIT(sw);
    P_INIT(fence_i);
    P_INIT(fence);

    P_INIT(lui);
    P_INIT(auipc);

    P_INIT(ecall);
    P_INIT(ebreak);

    // Custom instructions
    P_INIT(add16);
    P_INIT(add8);
    P_INIT(sub16);
    P_INIT(sub8);

    P_INIT(qadd16);
    P_INIT(qadd8);
    P_INIT(qadd16u);
    P_INIT(qadd8u);
    P_INIT(qsub16);
    P_INIT(qsub8);
    P_INIT(qsub16u);
    P_INIT(qsub8u);

    P_INIT(wmul16);
    P_INIT(wmul16u);
    P_INIT(wmul8);
    P_INIT(wmul8u);

    P_INIT(dot16);
    P_INIT(dot16u);
    P_INIT(dot8);
    P_INIT(dot8u);
    P_INIT(dot4);
    P_INIT(dot4u);
    P_INIT(dot2);
    P_INIT(dot2u);

    P_INIT(min16);
    P_INIT(min8);
    P_INIT(min16u);
    P_INIT(min8u);
    P_INIT(max16);
    P_INIT(max8);
    P_INIT(max16u);
    P_INIT(max8u);

    P_INIT(slli16);
    P_INIT(slli8);
    P_INIT(srli16);
    P_INIT(srli8);
    P_INIT(srai16);
    P_INIT(srai8);

    P_INIT(widen16);
    P_INIT(widen16u);
    P_INIT(widen8);
    P_INIT(widen8u);
    P_INIT(widen4);
    P_INIT(widen4u);
    P_INIT(widen2);
    P_INIT(widen2u);

    P_INIT(narrow32);
    P_INIT(narrow16);
    P_INIT(narrow8);
    P_INIT(narrow4);

    P_INIT(qnarrow32);
    P_INIT(qnarrow32u);
    P_INIT(qnarrow16);
    P_INIT(qnarrow16u);
    P_INIT(qnarrow8);
    P_INIT(qnarrow8u);
    P_INIT(qnarrow4);
    P_INIT(qnarrow4u);

    P_INIT(swapad16);
    P_INIT(swapad8);
    P_INIT(swapad4);
    P_INIT(swapad2);

    P_INIT(dup16);
    P_INIT(dup8);
    P_INIT(dup4);
    P_INIT(dup2);

    P_INIT(vins16);
    P_INIT(vins8);
    P_INIT(vins4);
    P_INIT(vins2);

    P_INIT(vext16);
    P_INIT(vext16u);
    P_INIT(vext8);
    P_INIT(vext8u);
    P_INIT(vext4);
    P_INIT(vext4u);
    P_INIT(vext2);
    P_INIT(vext2u);

    P_INIT_D(scp, lcl);
    P_INIT_D(scp, rel);

    // Zicsr extension
    P_INIT(csrrw);
    P_INIT(csrrs);
    P_INIT(csrrc);
    P_INIT(csrrwi);
    P_INIT(csrrsi);
    P_INIT(csrrci);

    // M extension
    P_INIT(mul);
    P_INIT(mulh);
    P_INIT(mulhsu);
    P_INIT(mulhu);
    P_INIT(div);
    P_INIT(divu);
    P_INIT(rem);
    P_INIT(remu);

    // Zbb extension
    P_INIT(max);
    P_INIT(maxu);
    P_INIT(min);
    P_INIT(minu);

    // C extension
    P_INIT_D(c, add);
    P_INIT_D(c, mv);
    P_INIT_D(c, and);
    P_INIT_D(c, or);
    P_INIT_D(c, xor);
    P_INIT_D(c, sub);
    P_INIT_D(c, addi);
    P_INIT_D(c, addi16sp);
    P_INIT_D(c, addi4spn);
    P_INIT_D(c, andi);
    P_INIT_D(c, srli);
    P_INIT_D(c, slli);
    P_INIT_D(c, srai);
    P_INIT_D(c, nop);
    P_INIT_D(c, lwsp);
    P_INIT_D(c, swsp);
    P_INIT_D(c, lw);
    P_INIT_D(c, sw);
    P_INIT_D(c, li);
    P_INIT_D(c, lui);
    P_INIT_D(c, ebreak);

    #undef P_INIT
    #undef P_INIT_D

    // ctrl
    #define P_INIT(s) \
        prof_b_arr[TO_U32(opc_b::i_##s)] = {#s, 0, 0, 0, 0}

    #define P_INIT_D(s1, s2) \
        prof_b_arr[TO_U32(opc_b::i_##s1##_##s2)] = {#s1"."#s2, 0, 0, 0, 0}

    P_INIT(beq);
    P_INIT(bne);
    P_INIT(blt);
    P_INIT(bge);
    P_INIT(bltu);
    P_INIT(bgeu);
    P_INIT(jalr);
    P_INIT(jal);

    // C extension ctrl
    P_INIT_D(c, j);
    P_INIT_D(c, jal);
    P_INIT_D(c, jr);
    P_INIT_D(c, jalr);
    P_INIT_D(c, beqz);
    P_INIT_D(c, bnez);

    #undef P_INIT
    #undef P_INIT_D
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
    for (auto &m: mul_c_opcs) cnt.wmul_c += prof_g_arr[TO_U32(m)].count;
    for (auto &z: zbb_opcs) cnt.zbb += prof_g_arr[TO_U32(z)].count;
    for (auto &u: widen_c_opcs) cnt.widen_c += prof_g_arr[TO_U32(u)].count;
    for (auto &u: narrow_c_opcs) cnt.narrow_c += prof_g_arr[TO_U32(u)].count;
    for (auto &u: swapad_c_opcs) cnt.swapad_c += prof_g_arr[TO_U32(u)].count;
    for (auto &u: dup_c_opcs) cnt.dup_c += prof_g_arr[TO_U32(u)].count;
    for (auto &u: vins_c_opcs) cnt.vins_c += prof_g_arr[TO_U32(u)].count;
    for (auto &u: vext_c_opcs) cnt.vext_c += prof_g_arr[TO_U32(u)].count;
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
    perc.wmul_c = cnt.get_perc(cnt.wmul_c);
    perc.zbb = cnt.get_perc(cnt.zbb);
    perc.widen_c = cnt.get_perc(cnt.widen_c);
    perc.narrow_c = cnt.get_perc(cnt.narrow_c);
    perc.swapad_c = cnt.get_perc(cnt.swapad_c);
    perc.dup_c = cnt.get_perc(cnt.dup_c);
    perc.vins_c = cnt.get_perc(cnt.vins_c);
    perc.vext_c = cnt.get_perc(cnt.vext_c);
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

    std::cout << INDENT << "SIMD arith:"
              << " ALU: " << cnt.alu_c << "(" << perc.alu_c << "%),"
              << " WMUL: " << cnt.wmul_c << "(" << perc.wmul_c << "%),"
              << " DOT: " << cnt.dot_c << "(" << perc.dot_c << "%)"
              << "\n";

    std::cout << INDENT << "SIMD data fmt:"
              << " WIDEN: " << cnt.widen_c << "(" << perc.widen_c << "%),"
              << " NARROW: " << cnt.narrow_c << "(" << perc.narrow_c << "%),"
              << " SWAPAD: " << cnt.swapad_c << "(" << perc.swapad_c << "%)"
              << "\n";

    std::cout << INDENT << "SIMD vector-scalar:"
              << " DUP: " << cnt.dup_c << "(" << perc.dup_c << "%),"
              << " VINS: " << cnt.vins_c << "(" << perc.vins_c << "%),"
              << " VEXT: " << cnt.vext_c << "(" << perc.vext_c << "%)"
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

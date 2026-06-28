#include "core.h"

core::core(memory *mem, cfg_t cfg, [[maybe_unused]] hw_cfg_t hw_cfg) :
    mem(mem),
    pc(mem_map::base_addr),
    tu(this, &core::trap_state_update_cb, pc, inst),
    out_dir(cfg.out_dir)
    #ifdef PROFILERS_EN
    , prof_pc(cfg.prof_pc)
    , prof(cfg.out_dir, PROF_SRC)
    , prof_perf(cfg.out_dir, mem->get_symbol_map(), cfg.perf_event, PROF_SRC)
    #endif
    #ifdef HW_MODELS_EN
    , bp_name("bpred")
    , bp(bp_name, hw_cfg)
    , div(hw_cfg.div_cache_entries)
    , no_bp(hw_cfg.bp_active == bp_t::none)
    #endif
{
    rf[0] = 0;
    rf_names_idx = TO_U8(cfg.rf_names);
    rf_names_w = (cfg.rf_names == rf_names_t::mode_abi) ? 4 : 3;
    for (uint32_t i = 1; i < 32; i++) rf[i] = 0xc0ffee;

    this->cfg = cfg;
    mem->trap_setup(&tu);

    #ifdef PROFILERS_EN
    prof.set_trace_en(cfg.prof_trace);
    if (cfg.no_callstack) prof_perf.set_callstack_en(false);
    prof_trace = cfg.prof_trace;
    prof_rf.set_trace_en(cfg.prof_trace);
    prof_rf.set_rf_usage_en(cfg.rf_usage);

    #ifdef DPI
    prof_perf.set_clk_src(&clk_src);
    // profiling active from the beginning instead of waiting for first PC exec
    bool prof_on_boot = (prof_pc.start == mem_map::base_addr);
    prof_active = prof_on_boot;
    prof.set_active(prof_on_boot);
    prof_perf.set_active(prof_on_boot);
    prof_fusion.set_active(prof_on_boot);
    prof_rf.set_active(prof_on_boot);
    cosim_prof(prof_on_boot);

    #endif // DPI
    #endif // PROFILERS_EN

    #ifdef DASM_EN
    #ifdef HW_MODELS_EN
    mem->set_hwmi(&hwmi);
    #endif
    tu.set_dasm(&dasm);
    logf = {cfg.log, cfg.log_always, cfg.log_always, cfg.log_state};
    logf.activate(false);
    if (logf.en) log_ofstream.open(cfg.out_dir + "exec.log");
    #ifdef PROFILERS_EN
    if (logf.act && prof_perf.is_callstack_en()) LOG_SYMBOL_TO_FILE;
    #endif
    #endif

    #ifdef HW_MODELS_EN
    last_inst_branch = false;
    mem->set_cache_hws(&hwrs.ic_hm, &hwrs.dc_hm);
    #ifndef PROFILERS_EN
    prof_state(true); // start profiling from boot, no profilers
    #else
    mem->set_perf_profiler(&prof_perf);
    #endif
    #endif

    #ifdef DPI
    static_assert(csr_rtl_trusted_all_supported(),
        "every csr_rtl_trusted entry must be present in supported_csrs");
    #endif

    // initialize CSRs
    csr_names_w = 0;
    for (const auto &c : csr_def::supported_csrs) {
        csr.insert({
            c.addr, csr_def::CSR(c.name, c.boot_val, c.perm, c.wmask, c.s_addr)
        });
        csr_names_w = std::max(csr_names_w, TO_U8(strlen(c.name)));
    }
    mem->set_mip(&csr.at(csr_map::addr::mip).value);
}

uint64_t core::run() {
    std::cout << std::dec << "SIMULATION STARTED\n";
    #ifdef PROFILERS_EN
    prof_active = false;
    prof.set_active(false);
    prof_perf.set_active(false);
    prof_fusion.set_active(false);
    prof_rf.set_active(false);
    #endif

    #ifdef UART_EN
    if (cfg.print_uart) std::cout << "=== UART START ===" << "\n";
    #endif

    // start the core
    running = true;
    while (running) single_step();

    // wrap up
    csr_cnt_update(0u); // so all instructions since last CSR access are counted
    finish(true);
    return sim_cnt.inst;
}

void core::single_step() {
    tu.clear_trap();

    // clear everything from previous instruction
    #ifdef DASM_EN
    dasm.clear_str();
    #ifdef HW_MODELS_EN
    hwmi.clear_str();
    #endif
    #endif

    #ifdef HW_MODELS_EN
    hwrs.rst();
    #endif

    #ifdef PROFILERS_EN
    if (prof_pc.should_start(pc)) {
        prof_state(true);
    } else if (prof_pc.should_stop(pc)) {
        prof_state(false);
        if (prof_pc.should_exit(pc)) {
            running = false;
            return;
        }
    }
    #endif

    // match RTL behavior, wfi in decode has priority over trap
    bool about_to_wfi = (
        !tu.is_trapped() && !wfi.active && (mem->just_inst(pc) == inst::wfi)
    );

    // if ((mtie && mtip) || (meie && meip)) -> no-trap interrupt wakeup
    bool no_trap_interrupt = check_interrupts(about_to_wfi);

    // clear wfi on interrupt (either trapped or not)
    wfi.active &= (!(tu.is_trapped() || no_trap_interrupt));
    if (!tu.is_trapped() && (!wfi.active || no_trap_interrupt)) {
        fetch();
        #ifdef PROFILERS_EN
        prof.new_inst(inst);
        branch_taken = false;
        #endif
        exec();
    }

    sim_cnt.step++;
    if (sim_cnt.step == cfg.run_steps) {
        std::cout << "Max simulation steps count " << sim_cnt.step
                  << " reached. Exiting.\n";
        running = false;
        return;
    }

    if (wfi.active && !tu.is_trapped()) return;

    #ifdef PROFILERS_EN
    [[maybe_unused]] bool log_symbol = false;
    if (!tu.is_trapped()){
        log_symbol = prof_perf.finish_inst(next_pc);
        if (prof_active) prof_pc.inst_cnt++;
    }
    #endif

    #ifdef DASM_EN
    // dasm string always available, logged to the file conditionally
    DASM_ALIGN;
    dasm.finish_inst();
    if (logf.act) log_inst(tu.is_trapped(), log_symbol);
    #endif

    if (tu.is_trapped()) {
        if (cfg.exit_on_trap) {
            std::cout << "Core trapped with exit_on_trap set. Exiting.\n";
            running = false;
        }
        return;
    }

    #ifdef PROFILERS_EN
    // rf changes only on retired instructions, no need to oversample from dpi
    prof.track_sp(rf[2]);
    #ifndef DPI // dpi calls its own save_trace_entry on every cycle
    save_trace_entry();
    #endif
    #endif

    #if defined(PROFILERS_EN) && defined(DASM_EN)
    if (log_symbol && logf.act) LOG_SYMBOL_TO_FILE;
    #endif

    pc = next_pc;
    sim_cnt.inst++;

    // transfer pending to active, and clear
    wfi.active |= wfi.pend;
    wfi.pend = false;

    if (sim_cnt.inst == cfg.run_insts) {
        std::cout << "Max simulation instructions count " << sim_cnt.inst
                    << " reached. Exiting.\n";
        running = false;
    }
}

bool core::check_interrupts(bool defer_trap) {
    if (defer_trap) return false; // will get them next time

    // nothing to update under DPI mode, RTL drives them
    #ifndef DPI
    mem->update_mtime();
    #ifdef UART_INPUT_EN
    mem->update_uart_input(sim_cnt.inst);
    #endif
    #endif

    uint32_t mie_val = csr.at(csr_map::addr::mie).value;
    uint32_t mip_val = csr.at(csr_map::addr::mip).value;
    bool mstatus_MIE = (csr.at(csr_map::addr::mstatus).value & csr_map::mstatus::mie);
    bool mti = ((mie_val & csr_map::mie::mtie) && (mip_val & csr_map::mip::mtip));
    bool mti_no_trap = (mti && !mstatus_MIE);

    #ifdef UART_INPUT_EN
    // priv spec: external (MEI) before timer (MTI), one trap per step
    // UART RX is the sole MEI source, so MEI lives under UART_INPUT_EN
    bool mei = ((mie_val & csr_map::mie::meie) && (mip_val & csr_map::mip::meip));
    bool mei_no_trap = (mei && !mstatus_MIE);
    if (mstatus_MIE && mei) {
        tu.e_external_interrupt();
        #ifdef DPI
        // one-shot; TB re-forces per RTL trap
        csr.at(csr_map::addr::mip).value &= ~csr_map::mip::meip;
        #endif
        #ifdef HW_MODELS_EN
        last_inst_branch = false;
        #endif
    } else
    #endif
    if (mstatus_MIE && mti) {
        tu.e_timer_interrupt();
        #ifdef DPI
        // one-shot; TB re-forces per RTL trap
        csr.at(csr_map::addr::mip).value &= ~csr_map::mip::mtip;
        #endif
        #ifdef HW_MODELS_EN
        last_inst_branch = false;
        #endif
    }
    return (
        mti_no_trap
        #ifdef UART_INPUT_EN
        || mei_no_trap
        #endif
    );
}

void core::fetch() {
    #ifdef HW_MODELS_EN
    // if previous inst was branch, use that instead of fetching
    // this prevents cache from logging the same access twice
    if (!last_inst_branch) {
        inst = mem->rd_inst(pc);
    } else {
        inst = inst_resolved;
        hwrs.ic_hm = next_ic_hm;
        next_ic_hm = hw_status_t::none;
    }
    last_inst_branch = false;
    #else // !HW_MODELS_EN
    inst = mem->rd_inst(pc);
    #endif
    ip.set(inst);
}

void core::exec() {
    uint32_t op_c = ip.copcode();
    if (op_c != 0x3) { // d_d_compressed ISA
        #ifdef RV32C
        INST_HEX_W(4);
        inst = ip.to_rvc(inst);
        switch (op_c) {
            case 0x0: d_compressed_0(); break;
            case 0x1: d_compressed_1(); break;
            case 0x2: d_compressed_2(); break;
            default: tu.e_unsupported_inst("op_c unreachable");
        }
        #else // !RV32C
        tu.e_unsupported_inst("RV32C unsupported");
        #endif

    } else { // 32-bit ISA
        INST_HEX_W(8);
        switch (ip.opcode()) {
            CASE_DECODER(d_alu_reg)
            CASE_DECODER(d_alu_imm)
            CASE_DECODER(d_load)
            CASE_DECODER(d_store)
            CASE_DECODER(d_branch)
            CASE_DECODER(d_jalr)
            CASE_DECODER(d_jal)
            CASE_DECODER(d_lui)
            CASE_DECODER(d_auipc)
            CASE_DECODER(d_system)
            CASE_DECODER(d_misc_mem)
            CASE_DECODER(d_custom_ext)
            default: tu.e_unsupported_inst("opcode");
        }
    }
}

#ifdef DASM_EN
void core::log_inst(bool trapped, bool log_symbol) {
    if (trapped) {
        // log changed callstack and return
        log_ofstream << dasm.asm_str << "\n";
        #ifdef DEBUG
        log_ofstream << std::flush;
        #endif

        #ifdef PROFILERS_EN
        if (log_symbol) LOG_SYMBOL_TO_FILE;
        #endif

        if (cfg.exit_on_trap) running = false;
        return;
    }

    log_ofstream << INDENT << std::setw(6) << std::setfill(' ');
    // don't count instructions unless also profiling
    //if (prof_active) log_ofstream << prof_pc.inst_cnt;
    #ifdef PROFILERS_EN
    if (prof_active) log_ofstream << sim_cnt.inst + 1;
    else log_ofstream << "";
    #else
    log_ofstream << sim_cnt.inst + 1;
    #endif
    log_ofstream << ": " << FORMAT_INST(pc, inst, inst_w) << " "
                    << dasm.asm_str;

    #ifdef HW_MODELS_EN
    if (cfg.log_hw_models) log_ofstream << hwmi.get_str();
    #endif

    log_ofstream << "\n";
    #ifdef DEBUG
    log_ofstream << std::flush;
    #endif

    if (logf.state) {
        log_ofstream << print_state(dasm_update_csr) << "\n";
        dasm_update_csr = false;
    }
}
#endif

void core::finish(bool dump_regs) {
    if (dump_regs) dump();
    if ((cfg.mem_dump_start > 0) && (cfg.mem_dump_size > 0)) {
        mem->dump_as_words(cfg.mem_dump_start, cfg.mem_dump_size, cfg.out_dir);
    }
    #ifdef PROFILERS_EN
    prof.finish(cfg.silent);
    prof_perf.finish(cfg.silent);
    prof_fusion.finish(cfg.silent);
    prof_rf.finish(cfg.out_dir);
    #endif
    #ifdef HW_MODELS_EN
    #ifdef PROFILERS_EN
    bp.finish(cfg.out_dir, prof_pc.inst_cnt, cfg.silent);
    mem->cache_finish(cfg.silent, prof_pc.inst_cnt);
    div.finish(cfg.silent);
    #else
    bp.finish(cfg.out_dir, sim_cnt.inst, cfg.silent);
    mem->cache_finish(cfg.silent, sim_cnt.inst);
    div.finish(cfg.silent);
    #endif
    log_hw_stats();
    #endif
    #ifdef DASM_EN
    log_ofstream << std::endl; // flush
    #endif
}

// profiler-related
void core::prof_state([[maybe_unused]] bool enable) {
    #ifdef PROFILERS_EN
    prof_active = enable;
    prof.set_active(enable);
    prof_perf.set_active(enable);
    prof_fusion.set_active(enable);
    prof_rf.set_active(enable);
    #ifdef DPI
    cosim_prof(enable); // updates bp, cache, core stats
    #endif
    #endif

    #ifdef DASM_EN
    logf.activate(enable);
    #ifdef PROFILERS_EN
    if (logf.act && prof_perf.is_callstack_en()) LOG_SYMBOL_TO_FILE;
    #endif
    #endif

    #ifdef HW_MODELS_EN
    bp.profiling(enable);
    div.profiling(enable);
    mem->cache_profiling(enable);
    hwrs.rst();
    #endif
}

#if defined(PROFILERS_EN) && !defined(DPI)
void core::save_trace_entry() {
    if (prof_active && prof_trace) {
        prof.te.inst = inst;
        prof.te.pc = pc;
        prof.te.next_pc = next_pc;
        prof.te.sp = rf[2];
        prof.te.taken = branch_taken;
        prof.te.inst_size = TO_U8(inst_w >> 1); // hex digits to bytes
        //prof.te.sample_cnt = prof_pc.inst_cnt;
        prof.te.sample_cnt = sim_cnt.inst + 1;
        #ifdef HW_MODELS_EN
        prof.te.ic_hm = TO_U8(hwrs.ic_hm);
        prof.te.dc_hm = TO_U8(hwrs.dc_hm);
        prof.te.bp_hm = TO_U8(hwrs.bp_hm);
        #endif
        prof.add_te();
        prof_rf.add_te();
    }
}
#endif

#ifdef DPI
// trace is saved on every cycle from dpi
void core::save_trace_entry(trace_entry te) {
    if (prof_active && prof_trace) {
        prof.te = te;
        prof.add_te();
    }
}
#endif

// Integer extension - decoders
void core::d_alu_reg() {
    uint32_t funct7 = ip.funct7();
    uint32_t funct3 = ip.funct3();
    uint32_t alu_op_sel = (((ip.funct7_b5()) << 3) | funct3);
    uint32_t res;
    switch(funct7) {
        case 0x00: // rv32i
        case 0x20: // rv32i sub and sra
            switch (alu_op_sel) {
                CASE_ALU_REG_OP(add)
                CASE_ALU_REG_OP(sub)
                CASE_ALU_REG_OP(sll)
                CASE_ALU_REG_OP(srl)
                CASE_ALU_REG_OP(sra)
                CASE_ALU_REG_OP(slt)
                CASE_ALU_REG_OP(sltu)
                CASE_ALU_REG_OP(xor)
                CASE_ALU_REG_OP(or)
                CASE_ALU_REG_OP(and)
                default: tu.e_unsupported_inst("alu_reg_rv32i"); return;
            }
            break;
        case 0x01: // rv32m
            switch (funct3) {
                CASE_ALU_REG_MUL_OP(mul)
                CASE_ALU_REG_MUL_OP(mulh)
                CASE_ALU_REG_MUL_OP(mulhsu)
                CASE_ALU_REG_MUL_OP(mulhu)
                CASE_ALU_REG_DIV_OP(div, false)
                CASE_ALU_REG_DIV_OP(divu, true)
                CASE_ALU_REG_DIV_OP(rem, false)
                CASE_ALU_REG_DIV_OP(remu, true)
                default: tu.e_unsupported_inst("alu_reg_rv32m"); return;
            }
            break;
        case 0x05: // rv32 zbb
            switch (funct3) {
                CASE_ALU_REG_ZBB_OP(max)
                CASE_ALU_REG_ZBB_OP(maxu)
                CASE_ALU_REG_ZBB_OP(min)
                CASE_ALU_REG_ZBB_OP(minu)
                default: tu.e_unsupported_inst("alu_reg_rv32_zbb"); return;
            }
            break;
        default: tu.e_unsupported_inst("alu_reg"); return;
    }
    next_pc = pc + 4;
    #ifdef DASM_EN
    DASM_OP_RD << "," << DASM_OP_RS1 << "," << DASM_OP_RS2;
    DASM_RD_UPDATE;
    #endif
}

void core::d_alu_imm() {
    uint32_t alu_op_sel_shift = ((ip.funct7_b5()) << 3) | ip.funct3();
    bool is_shift = (ip.funct3() & 0x3) == 1;
    uint32_t alu_op_sel = is_shift ? alu_op_sel_shift : ip.funct3();
    uint32_t res;
    switch (alu_op_sel) {
        CASE_ALU_IMM_OP(addi)
        CASE_ALU_IMM_OP(slli)
        CASE_ALU_IMM_OP(srli)
        CASE_ALU_IMM_OP(srai)
        CASE_ALU_IMM_OP(slti)
        CASE_ALU_IMM_OP(sltiu)
        CASE_ALU_IMM_OP(xori)
        CASE_ALU_IMM_OP(ori)
        CASE_ALU_IMM_OP(andi)
        default: tu.e_unsupported_inst("alu_imm"); return;
    }
    next_pc = pc + 4;
    #ifdef DASM_EN
    DASM_OP_RD << "," << DASM_OP_RS1 << ",";
    if (is_shift) dasm.asm_ss << FHEXN(ip.imm_i_shamt(), 2);
    else dasm.asm_ss << TO_I32(ip.imm_i());
    DASM_RD_UPDATE;
    if (inst == inst::nop) {
        dasm.clear_str();
        dasm.asm_ss << "nop";
    }
    #endif
}

void core::d_load() {
    #if defined(PROFILERS_EN) || defined(DASM_EN)
    uint32_t rs1 = rf[ip.rs1()];
    #endif
    uint32_t loaded;
    switch (ip.funct3()) {
        CASE_LOAD(lb)
        CASE_LOAD(lh)
        CASE_LOAD(lw)
        CASE_LOAD(lbu)
        CASE_LOAD(lhu)
        default: tu.e_unsupported_inst("load"); return;
    }
    next_pc = pc + 4;

    #ifdef PROFILERS_EN
    prof.log_stack_access_load((rs1 + ip.imm_i()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef DASM_EN
    DASM_OP_RD << "," << TO_I32(ip.imm_i()) << "(" << DASM_OP_RS1 << ")";
    DASM_RD_UPDATE;
    if (ip.rd()) {
        dasm.asm_ss << " <- mem["
                    << MEM_ADDR_FORMAT(TO_I32(ip.imm_i()) + rs1) << "]";
    }
    #endif
}

void core::d_store() {
    uint32_t addr;
    switch (ip.funct3()) {
        CASE_STORE(sb)
        CASE_STORE(sh)
        CASE_STORE(sw)
        default: tu.e_unsupported_inst("store");
    }
    #ifdef PROFILERS_EN
    prof.log_stack_access_store((rf[ip.rs1()] + ip.imm_s()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    next_pc = pc + 4;
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << DASM_OP_RS2 << "," << TO_I32(ip.imm_s())
                << "(" << DASM_OP_RS1 << ")";
    DASM_MEM_UPDATE;
    #endif
}

void core::d_branch() {
    [[maybe_unused]] bool taken = false; // unused if built without PROFILERS_EN
    next_pc = (pc + 4); // updated in CASE_BRANCH if taken
    uint32_t target_pc = (pc + ip.imm_b());
    switch (ip.funct3()) {
        CASE_BRANCH(beq)
        CASE_BRANCH(bne)
        CASE_BRANCH(blt)
        CASE_BRANCH(bge)
        CASE_BRANCH(bltu)
        CASE_BRANCH(bgeu)
        default: tu.e_unsupported_inst("branch");
    }

    #ifndef RV32C
    bool address_unaligned = (next_pc % 4 != 0);
    if (address_unaligned) {
        tu.e_inst_addr_misaligned(next_pc, "branch unaligned access");
        return;
    }
    #endif

    #ifdef PROFILERS_EN
    prof_perf.update_branch(next_pc, taken);
    prof_perf.set_perf_event_flag(perf_event_t::branch);
    branch_taken = taken;
    #endif

    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << DASM_OP_RS1 << "," << DASM_OP_RS2 << ","
                << std::hex << pc + TO_I32(ip.imm_b()) << std::dec;
    #endif

    #ifdef HW_MODELS_EN
    // buffer icache hit/miss; at least 1 if bp hits, but 2 if bp misses
    hw_status_t save_ic_hm = hwrs.ic_hm; // next fetch will override the stat
    last_inst_branch = true;
    bp.ideal(next_pc);

    uint32_t speculative_next_pc = bp.predict(pc, ip.imm_b(), ip.funct3());
    if (!no_bp) { // not going to icache in case there is no bp, stall instead
        //mem->speculative_exec(speculative_t::enter);
        inst_speculative = mem->rd_inst(speculative_next_pc);
        // count speculative I$ reference, expected correct most of the time
        next_ic_hm = hwrs.ic_hm;
    }

    bp.update(pc, next_pc);
    bool correct = (next_pc == speculative_next_pc);
    if (!correct) {
        //mem->speculative_exec(speculative_t::exit_flush);
        inst_resolved = mem->rd_inst(next_pc);
        // this is another I$ reference, after the speculative fetch above
        // but trace can't store more than 1 reference for the same instruction
        // 1. don't count this [USED]
        // 2. count the worst of the two (i.e. miss if either is miss)
        // 3. count only resolution, as the previous miss might not be served
        // if no bp, this I$ reference is counted, no bp has only resolution
        if (no_bp) next_ic_hm = hwrs.ic_hm;
        #ifdef PROFILERS_EN
        prof_perf.set_perf_event_flag(perf_event_t::bp_mispredict);
        #endif
    } else {
        //mem->speculative_exec(speculative_t::exit_commit);
        inst_resolved = inst_speculative;
    }

    hwrs.bp_hm = static_cast<hw_status_t>(correct);
    hwrs.ic_hm = save_ic_hm;

    #ifdef DASM_EN
    hwmi.log_bp({bp_name, b_dir_t::forward, correct, taken});
    #endif

    #endif // HW_MODELS_EN
}

void core::d_jalr() {
    jalr();
    DASM_OP(jalr)
    PROF_J(jalr)
    PROF_RD_RS1

    #if defined(PROFILERS_EN) || defined(DASM_EN)
    bool ret_inst = (
        (inst == inst::heuristic::ret) || (inst == inst::heuristic::ret_x5)
    ); // but not X15
    #endif

    #ifdef PROFILERS_EN
    bool tail_call = (ip.rd() == 0);
    prof_perf.update_jalr(next_pc, ret_inst, tail_call, (pc + 4));
    branch_taken = true;
    #endif

    #ifdef DASM_EN
    DASM_OP_RD << "," << TO_I32(ip.imm_i()) << "(" << DASM_OP_RS1 << ")";
    if (ret_inst) dasm.asm_ss << " # ret";
    DASM_RD_UPDATE;
    #endif
}

void core::d_jal() {
    jal();
    DASM_OP(jal)
    PROF_J(jal)
    PROF_RD

    #ifdef PROFILERS_EN
    //bool pc_match = (pc == 0x19118); // known noreturn call
    bool tail_call = (ip.rd() == 0); // || pc_match;
    prof_perf.update_jal(next_pc, tail_call, (pc + 4));
    branch_taken = true;
    #endif

    #ifdef DASM_EN
    DASM_OP_RD << "," << std::hex <<( pc + TO_I32(ip.imm_j())) << std::dec;
    DASM_RD_UPDATE;
    #endif
}

void core::d_lui() {
    lui();
    next_pc = (pc + 4);
    DASM_OP(lui)
    PROF_G(lui)
    PROF_RD
    #ifdef DASM_EN
    DASM_OP_RD << ", " << FHEXN((ip.imm_u() >> 12), 5);
    DASM_RD_UPDATE;
    #endif
}

void core::d_auipc() {
    auipc();
    next_pc = (pc + 4);
    DASM_OP(auipc)
    PROF_G(auipc)
    PROF_RD
    #ifdef DASM_EN
    DASM_OP_RD << ", " << FHEXN((ip.imm_u() >> 12), 5);
    DASM_RD_UPDATE;
    #endif
}

void core::d_system() {
    uint32_t funct3 = ip.funct3();
    if (funct3) {
        d_csr_access();
        next_pc = pc + 4;
    } else { // (funct3 == 0) -> system instructions
        switch (inst) {
            case inst::ecall:
                tu.e_env("ECALL", csr_map::mcause::machine_ecall);
                return;
            case inst::ebreak:
                tu.e_env("EBREAK", csr_map::mcause::breakpoint);
                return;
            case inst::mret:
                mret_state_update();
                DASM_OP(mret)
                PROF_G(mret)
                #ifdef PROFILERS_EN
                prof_perf.update_jalr(next_pc, true, false, 0u);
                #endif
                break;
            case inst::wfi:
                next_pc = (pc + 4);
                wfi.pend = true;
                DASM_OP(wfi)
                PROF_G(wfi)
                break;
            default: tu.e_unsupported_inst("system");
        }
        #ifdef DASM_EN
        dasm.asm_ss << dasm.op;
        #endif
    }
}

void core::d_misc_mem() {
    if (inst == inst::fence_i) {
        // nop
        next_pc = pc + 4;
        DASM_OP(fence.i)
        PROF_G(fence_i)
    } else if (ip.funct3() == 0) {
        // nop
        next_pc = pc + 4;
        DASM_OP(fence)
        PROF_G(fence)
    } else {
        tu.e_unsupported_inst("misc_mem");
    }
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op;
    #endif
}

void core::d_custom_ext() {
    uint8_t funct3 = ip.funct3();
    uint8_t funct7 = ip.funct7();
    uint32_t shamt = ip.rs2();
    uint32_t res;
    reg_pair rp;
    switch(funct7) {
        case TO_U8(custom_op_t::type_alu):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_ADDSUB_CUSTOM_OP(add16, alu)
                CASE_ADDSUB_CUSTOM_OP(add8, alu)
                CASE_ADDSUB_CUSTOM_OP(sub16, alu)
                CASE_ADDSUB_CUSTOM_OP(sub8, alu)
                default: tu.e_unsupported_inst("alu_addsub_custom"); return;
            }
            break;
        case TO_U8(custom_op_t::type_qalu):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_QADDSUB_CUSTOM_OP(qadd16, alu)
                CASE_QADDSUB_CUSTOM_OP(qadd8, alu)
                CASE_QADDSUB_CUSTOM_OP(qadd16u, alu)
                CASE_QADDSUB_CUSTOM_OP(qadd8u, alu)
                CASE_QADDSUB_CUSTOM_OP(qsub16, alu)
                CASE_QADDSUB_CUSTOM_OP(qsub8, alu)
                CASE_QADDSUB_CUSTOM_OP(qsub16u, alu)
                CASE_QADDSUB_CUSTOM_OP(qsub8u, alu)
                default: tu.e_unsupported_inst("alu_qaddsub_custom"); return;
            }
            break;
        case TO_U8(custom_op_t::type_mul):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_MUL_CUSTOM_OP(mul16, mul)
                CASE_MUL_CUSTOM_OP(mul8, mul)
                CASE_MUL_CUSTOM_OP(mulh16, mul)
                CASE_MUL_CUSTOM_OP(mulh16u, mul)
                CASE_MUL_CUSTOM_OP(mulh8, mul)
                CASE_MUL_CUSTOM_OP(mulh8u, mul)
                default: tu.e_unsupported_inst("alu_mul_custom"); return;
            }
            break;
        case TO_U8(custom_op_t::type_wmul):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_WMUL_CUSTOM_OP(wmul16, mul)
                CASE_WMUL_CUSTOM_OP(wmul16u, mul)
                CASE_WMUL_CUSTOM_OP(wmul8, mul)
                CASE_WMUL_CUSTOM_OP(wmul8u, mul)
                default: tu.e_unsupported_inst("alu_wmul_custom"); return;
            }
            break;
        case TO_U8(custom_op_t::type_dot):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_ALU_CUSTOM_DOT(dot16, dot)
                CASE_ALU_CUSTOM_DOT(dot16u, dot)
                CASE_ALU_CUSTOM_DOT(dot8, dot)
                CASE_ALU_CUSTOM_DOT(dot8u, dot)
                CASE_ALU_CUSTOM_DOT(dot4, dot)
                CASE_ALU_CUSTOM_DOT(dot4u, dot)
                CASE_ALU_CUSTOM_DOT(dot2, dot)
                CASE_ALU_CUSTOM_DOT(dot2u, dot)
                default : tu.e_unsupported_inst("alu_dot_custom");
            }
            break;
        case TO_U8(custom_op_t::type_min_max):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_MIN_MAX_CUSTOM_OP(min16, alu)
                CASE_MIN_MAX_CUSTOM_OP(min16u, alu)
                CASE_MIN_MAX_CUSTOM_OP(min8, alu)
                CASE_MIN_MAX_CUSTOM_OP(min8u, alu)
                CASE_MIN_MAX_CUSTOM_OP(max16, alu)
                CASE_MIN_MAX_CUSTOM_OP(max16u, alu)
                CASE_MIN_MAX_CUSTOM_OP(max8, alu)
                CASE_MIN_MAX_CUSTOM_OP(max8u, alu)
                default: tu.e_unsupported_inst("alu_min_max_custom"); return;
            }
            break;
        case TO_U8(custom_op_t::type_shift):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_SHIFT_CUSTOM_OP(slli16, alu)
                CASE_SHIFT_CUSTOM_OP(slli8, alu)
                CASE_SHIFT_CUSTOM_OP(srli16, alu)
                CASE_SHIFT_CUSTOM_OP(srli8, alu)
                CASE_SHIFT_CUSTOM_OP(srai16, alu)
                CASE_SHIFT_CUSTOM_OP(srai8, alu)
                default: tu.e_unsupported_inst("alu_shift_custom"); return;
            }
            break;
        case TO_U8(custom_op_t::type_data_fmt_widen):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_DATA_FMT_WIDEN_CUSTOM_OP(widen16)
                CASE_DATA_FMT_WIDEN_CUSTOM_OP(widen16u)
                CASE_DATA_FMT_WIDEN_CUSTOM_OP(widen8)
                CASE_DATA_FMT_WIDEN_CUSTOM_OP(widen8u)
                CASE_DATA_FMT_WIDEN_CUSTOM_OP(widen4)
                CASE_DATA_FMT_WIDEN_CUSTOM_OP(widen4u)
                CASE_DATA_FMT_WIDEN_CUSTOM_OP(widen2)
                CASE_DATA_FMT_WIDEN_CUSTOM_OP(widen2u)
                default: tu.e_unsupported_inst("data_fmt_widen_custom");
            }
            break;
        case TO_U8(custom_op_t::type_data_fmt_narrow):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_DATA_FMT_NARROW_CUSTOM_OP(narrow32)
                CASE_DATA_FMT_NARROW_CUSTOM_OP(narrow16)
                CASE_DATA_FMT_NARROW_CUSTOM_OP(narrow8)
                CASE_DATA_FMT_NARROW_CUSTOM_OP(narrow4)
                default: tu.e_unsupported_inst("data_fmt_narrow_custom");
            }
            break;
        case TO_U8(custom_op_t::type_data_fmt_qnarrow):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_DATA_FMT_QNARROW_CUSTOM_OP(qnarrow32)
                CASE_DATA_FMT_QNARROW_CUSTOM_OP(qnarrow32u)
                CASE_DATA_FMT_QNARROW_CUSTOM_OP(qnarrow16)
                CASE_DATA_FMT_QNARROW_CUSTOM_OP(qnarrow16u)
                CASE_DATA_FMT_QNARROW_CUSTOM_OP(qnarrow8)
                CASE_DATA_FMT_QNARROW_CUSTOM_OP(qnarrow8u)
                CASE_DATA_FMT_QNARROW_CUSTOM_OP(qnarrow4)
                CASE_DATA_FMT_QNARROW_CUSTOM_OP(qnarrow4u)
                default: tu.e_unsupported_inst("data_fmt_qnarrow_custom");
            }
            break;
        case TO_U8(custom_op_t::type_data_fmt_txp):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_DATA_FMT_TXP_CUSTOM_OP(txp16)
                CASE_DATA_FMT_TXP_CUSTOM_OP(txp8)
                CASE_DATA_FMT_TXP_CUSTOM_OP(txp4)
                CASE_DATA_FMT_TXP_CUSTOM_OP(txp2)
                default: tu.e_unsupported_inst("data_fmt_txp_custom");
            }
            break;
        case TO_U8(custom_op_t::type_sv_dup_vins):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_SV_DUP_CUSTOM_OP(dup16)
                CASE_SV_VINS_CUSTOM_OP(vins16, 0x1)
                CASE_SV_DUP_CUSTOM_OP(dup8)
                CASE_SV_VINS_CUSTOM_OP(vins8, 0x3)
                CASE_SV_DUP_CUSTOM_OP(dup4)
                CASE_SV_VINS_CUSTOM_OP(vins4, 0x7)
                CASE_SV_DUP_CUSTOM_OP(dup2)
                CASE_SV_VINS_CUSTOM_OP(vins2, 0xf)
                default: tu.e_unsupported_inst("sv_dup_vins_custom");
            }
            break;
        case TO_U8(custom_op_t::type_sv_vext):
            PROF_SET_PERF_EVENT_SIMD
            switch (funct3) {
                CASE_SV_VEXT_CUSTOM_OP(vext16, 0x1)
                CASE_SV_VEXT_CUSTOM_OP(vext16u, 0x1)
                CASE_SV_VEXT_CUSTOM_OP(vext8, 0x3)
                CASE_SV_VEXT_CUSTOM_OP(vext8u, 0x3)
                CASE_SV_VEXT_CUSTOM_OP(vext4, 0x7)
                CASE_SV_VEXT_CUSTOM_OP(vext4u, 0x7)
                CASE_SV_VEXT_CUSTOM_OP(vext2, 0xf)
                CASE_SV_VEXT_CUSTOM_OP(vext2u, 0xf)
                default: tu.e_unsupported_inst("sv_vext_custom");
            }
            break;
        case TO_U8(custom_op_t::type_hints):
            switch (funct3) {
                CASE_SCP_CUSTOM(lcl); DASM_OP(scp.lcl); break;
                CASE_SCP_CUSTOM(rel); DASM_OP(scp.rel); break;
                default : tu.e_unsupported_inst("hint_custom");
            }
            break;
        default: tu.e_unsupported_inst("custom_isa");
    }

    #ifdef DASM_EN
    switch(funct7) {
        case TO_U8(custom_op_t::type_alu):
        case TO_U8(custom_op_t::type_qalu):
        case TO_U8(custom_op_t::type_mul):
        case TO_U8(custom_op_t::type_dot):
        case TO_U8(custom_op_t::type_min_max):
        case TO_U8(custom_op_t::type_data_fmt_narrow):
        case TO_U8(custom_op_t::type_data_fmt_qnarrow):
            DASM_OP_RD << "," << DASM_OP_RS1 << "," << DASM_OP_RS2;
            DASM_RD_UPDATE;
            break;
        case TO_U8(custom_op_t::type_shift):
            DASM_OP_RD << "," << DASM_OP_RS1 << "," FHEXN(ip.rs2(), 2);
            DASM_RD_UPDATE;
            break;
        case TO_U8(custom_op_t::type_wmul):
        case TO_U8(custom_op_t::type_data_fmt_txp):
            DASM_OP_RD << "," << DASM_OP_RS1 << "," << DASM_OP_RS2;
            DASM_RD_UPDATE;
            DASM_RD_UPDATE_PAIR;
            break;
        case TO_U8(custom_op_t::type_data_fmt_widen):
            DASM_OP_RD << "," << DASM_OP_RS1 << "," FHEXN(ip.rs2(), 2);
            DASM_RD_UPDATE;
            DASM_RD_UPDATE_PAIR;
            break;
        case TO_U8(custom_op_t::type_sv_dup_vins):
            DASM_OP_RD << "," << DASM_OP_RS1;
            if (funct3 & 0x1) {
                // lsb set, vins (fn3 1,3,5,7 -> 1,2,3,4 bit mask)
                static const uint32_t vins_lane_mask[] = {
                    0x1u, 0x3u, 0x7u, 0xfu };
                uint32_t imm = (ip.rs2() & vins_lane_mask[funct3 >> 1]);
                dasm.asm_ss << "," << FHEXN(imm, 1);
            }
            DASM_RD_UPDATE;
            break;
        case TO_U8(custom_op_t::type_sv_vext):
            {
                // vext: fn3 0,1->16-bit; 2,3->8-bit; 4,5->4-bit; 6,7->2-bit
                static const uint32_t vext_lane_mask[] = {
                    0x1u, 0x3u, 0x7u, 0xfu };
                uint32_t imm = (ip.rs2() & vext_lane_mask[funct3 >> 1]);
                DASM_OP_RD << "," << DASM_OP_RS1 << "," << FHEXN(imm, 1);
                DASM_RD_UPDATE;
            }
            break;
        case TO_U8(custom_op_t::type_hints):
            DASM_OP_RD << "," << DASM_OP_RS1;
            DASM_RD_UPDATE;
            break;
    }
    #endif
    next_pc = pc + 4;
}

void core::d_csr_access() {
    uint16_t csr_addr = TO_U16(ip.csr_addr());
    auto it = csr.find(csr_addr);
    if (it == csr.end()) {
        tu.e_illegal_inst("CSR write attempt to non-existent CSR", 4);
        return;
    } else {
        #ifndef DPI
        csr_cnt_update(csr_addr);
        #endif
        // using temp in case rd and rs1 are the same register
        uint32_t init_val_rs1 = rf[ip.rs1()];
        // FIXME: rw/rwi should not read CSR on rd=x0; no impact w/ current CSRs
        #ifdef DPI
        uint32_t val = is_rtl_trusted(csr_addr) ?
            get_rtl_rf_value(ip.rd()) : it->second.value;
        write_rf(ip.rd(), val);
        #else
        write_rf(ip.rd(), it->second.value);
        #endif
        switch (ip.funct3()) {
            CASE_CSR(rw)
            CASE_CSR(rs)
            CASE_CSR(rc)
            CASE_CSR_I(rwi)
            CASE_CSR_I(rsi)
            CASE_CSR_I(rci)
            default: tu.e_unsupported_inst("sys");
        }
        if (csr.at(csr_map::addr::tohost).value & 0x1) running = false;
    }

    #ifdef DASM_EN
    bool imm_type = (ip.funct3() & 0x4);
    DASM_RD_UPDATE;
    if (ip.rs1() || imm_type) {
        if (ip.rd()) dasm.asm_ss << "; ";
        DASM_ALIGN;
        dasm.asm_ss << CSRF(it);
    }
    if (logf.state) dasm_update_csr = true;
    #endif
}

// C extension - decoders
void core::d_compressed_0() {
    uint32_t funct3 = ip.c_funct3();
    switch (funct3) {
        case 0x0: c_addi4spn(); break;
        case 0x2: c_lw(); break;
        case 0x6: c_sw(); break;
        default: tu.e_unsupported_inst("compressed_0");
    }
}

void core::d_compressed_1() {
    uint32_t funct3 = ip.c_funct3();
    uint32_t funct2h = ip.c_funct2h();
    uint32_t funct2l = ip.c_funct2l();
    uint32_t funct6 = ip.c_funct6();

    switch (funct3) {
        case 0x0: c_addi(); break;
        case 0x1: c_jal(); break;
        case 0x2: c_li(); break;
        case 0x3:
            switch (ip.rd()) {
                case 0x0: c_nop(); break;
                case 0x2: c_addi16sp(); break;
                default: c_lui(); break;
            }
            break;
        case 0x4:
            switch (funct2h) {
                case 0x0: c_srli(); break;
                case 0x1: c_srai(); break;
                case 0x2: c_andi(); break;
                case 0x3:
                    switch ((funct6 << 2) | funct2l) {
                        case 0x8c: c_sub(); break;
                        case 0x8d: c_xor(); break;
                        case 0x8e: c_or(); break;
                        case 0x8f: c_and(); break;
                        default: tu.e_unsupported_inst("compressed_1:0x4:0x3");
                    }
                    break;
            }
            break;
        case 0x5: c_j(); break;
        case 0x6: c_beqz(); break;
        case 0x7: c_bnez(); break;
        default: tu.e_unsupported_inst("compressed_1");
    }
}

void core::d_compressed_2() {
    uint32_t funct4 = ip.c_funct4();
    uint32_t funct3 = ip.c_funct3();
    switch (funct3) {
        case 0x0: c_slli(); break;
        case 0x2: c_lwsp(); break;
        case 0x6: c_swsp(); break;
        case 0x4:
            switch (funct4) {
                case 0x8:
                    if (ip.c_rs2() == 0x0) c_jr();
                    else c_mv();
                    break;
                case 0x9:
                    if (ip.c_rs2() == 0x0 && ip.rd() == 0x0) c_ebreak();
                    else if (ip.c_rs2() == 0x0) c_jalr();
                    else c_add();
                break;
                default: tu.e_unsupported_inst("unreachable");
            }
            break;
        default: tu.e_unsupported_inst("compressed_2");
    }
}

// HW stats
#ifdef HW_MODELS_EN
void core::log_hw_stats() {
    std::ofstream ofs;
    ofs.open(cfg.out_dir + "hw_stats.json");
    ofs << "{\n";
    #ifdef PROFILERS_EN
    mem->log_cache_stats(ofs, prof_pc.inst_cnt);
    #else
    mem->log_cache_stats(ofs, sim_cnt.inst);
    #endif
    bp.log_stats(ofs);
    div.log_stats("divider", ofs);
    ofs << "\n\"profiled_inst\": "
    #ifdef PROFILERS_EN
    << prof_pc.inst_cnt // profiled inst, depending on settings/triggers
    #else
    << sim_cnt.inst // app profiled from boot
    #endif
    << "\n}\n";
    ofs.close();
}
#endif

// Utilities
void core::dump() {
    #ifdef UART_EN
    if (cfg.print_uart) std::cout << "\n=== UART END ===\n";
    #endif
    std::cout << "SIMULATION FINISHED\n\n";

    #ifdef PROFILERS_EN
    if (prof_pc.exit_on_prof_stop) {
        std::cout << "Early exit on profiler stop, TOHOST invalid\n";
        csr.at(csr_map::addr::tohost).value = csr_def::tohost_early_exit;
    } else
    #endif
    {
        uint32_t tohost = csr.at(csr_map::addr::tohost).value;
        if (tohost != 1) {
            std::cout << "Failed test ID: " << (tohost >> 1) << " (0x"
                      << std::hex << (tohost >> 1) << std::dec << ")";
            std::cout << ((tohost > 1000) ? " (trap)" : " (exit)") << "\n";
        }
    }
    std::cout << std::dec << "Instruction Counters: executed: " << sim_cnt.inst
              #ifdef PROFILERS_EN
              // profiled inst, depending on settings/triggers
              << ", profiled: " << prof_pc.inst_cnt
              #endif
              << "\n";
    if (cfg.show_state) std::cout << print_state(true) << "\n";
    else std::cout << INDENT << CSRF(csr.find(csr_map::addr::tohost)) << "\n";

    #ifdef CHECK_LOG
    // open file for check log
    std::ofstream file;
    file.open("sim.check");
    file << std::dec << sim_cnt.inst << "\n";
    for(uint32_t i = 0; i < 32; i++) file << FHEXZ(rf[i], 8) << "\n";
    file << "0x" << MEM_ADDR_FORMAT(pc) << "\n";
    file << FHEXZ(csr.at(csr_map::addr::tohost).value, 8) << "\n";
    file.close();
    #endif
}

std::string core::print_state(bool dump_csr) {
    std::ostringstream state;
    state << INDENT_3X << "PC: " << MEM_ADDR_FORMAT(pc) << "\n";
    for(uint32_t i = 0; i < 32; i+=4){
        state << INDENT_3X;
        for(uint32_t j = 0; j < 4; j++) {
            state << FRF(rf_names[i+j][rf_names_idx], rf[i+j]) << "   ";
        }
        state << "\n";
    }
    if (dump_csr) {
        for (auto it = csr.begin(); it != csr.end(); it++) {
            state << INDENT_3X << CSRF(it) << "\n";
        }
    }

    return state.str();
}

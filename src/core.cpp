#include "core.h"

core::core(
    memory *mem,
    cfg_t cfg,
    [[maybe_unused]] hw_cfg_t hw_cfg) :
        running(false),
        mem(mem), pc(BASE_ADDR), next_pc(0), inst(0),
        inst_cnt(0), inst_cnt_csr(0),
        tu(&csr, &pc, &inst),
        out_dir(cfg.out_dir), prof_pc(cfg.prof_pc), prof_act(false)
    #ifdef PROFILERS_EN
    , prof(cfg.out_dir, PROF_SRC)
    , prof_perf(cfg.out_dir, mem->get_symbol_map(), cfg.perf_event, PROF_SRC)
    #endif
    #ifdef HW_MODELS_EN
    , bp(bp_name, hw_cfg)
    , no_bp(hw_cfg.bp_active == bp_t::none)
    #endif
{
    rf[0] = 0;
    rf_names_idx = TO_U8(cfg.rf_names);
    rf_names_w = cfg.rf_names == rf_names_t::mode_abi ? 4 : 3;
    end_dump_state = cfg.end_dump_state;
    for (uint32_t i = 1; i < 32; i++) rf[i] = 0xc0ffee;

    this->cfg = cfg;
    mem->trap_setup(&tu);
    #ifdef PROFILERS_EN
    tu.set_prof_perf(&prof_perf);
    prof.set_prof_flags(cfg.prof_trace, cfg.rf_usage);
    prof_trace = cfg.prof_trace;
    #ifdef DPI
    prof_perf.set_clk_src(&clk_src);
    #endif
    #endif

    #ifdef DASM_EN
    #ifdef HW_MODELS_EN
    mem->set_hwmi(&hwmi);
    #endif
    tu.set_dasm(&dasm);
    logf = {cfg.log, cfg.log_always, cfg.log_always, cfg.log_state};
    logf.activate(false);
    if (logf.en) log_ofstream.open(cfg.out_dir + "exec.log");
    if (logf.act) LOG_SYMBOL_TO_FILE;
    #endif

    #ifdef HW_MODELS_EN
    last_inst_branch = false;
    mem->set_cache_hws(&hwrs.ic_hm, &hwrs.dc_hm);
    #endif

    // initialize CSRs
    csr_names_w = 0;
    for (const auto &c : supported_csrs) {
        csr.insert({c.csr_addr, CSR(c.csr_name, c.boot_val, c.perm)});
        csr_names_w = std::max(csr_names_w, TO_U8(strlen(c.csr_name)));
    }
    mem->set_mip(&csr.at(CSR_MIP).value);

    #if defined(PROFILERS_EN) && defined(HW_MODELS_EN)
    mem->set_perf_profiler(&prof_perf);
    #endif
}

void core::exec() {
    std::cout << std::dec << "SIMULATION STARTED\n";
    #ifdef PROFILERS_EN
    prof_act = false;
    prof.active = false;
    prof_perf.active = false;
    prof_fusion.active = false;
    #endif

    #ifdef UART_EN
    if (!cfg.sink_uart) std::cout << "=== UART START ===" << "\n";
    #endif

    // start the core
    running = true;
    while (running) exec_inst();

    // wrap up
    cntr_update(); // so that all instructions since last CSR access are counted
    finish(true);
    return;
}

void core::prof_state([[maybe_unused]] bool enable) {
    #ifdef PROFILERS_EN
    prof_act = enable;
    prof.active = enable;
    prof_perf.active = enable;
    prof_fusion.active = enable;
    #endif

    #ifdef DASM_EN
    logf.activate(enable);
    if (logf.act) LOG_SYMBOL_TO_FILE;
    #endif

    #ifdef HW_MODELS_EN
    prof_act = enable;
    bp.profiling(enable);
    mem->cache_profiling(enable);
    hwrs.rst();
    #endif
}

void core::exec_inst() {
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

    if (prof_pc.should_start(pc)) {
        prof_state(true);
    } else if (prof_pc.should_stop(pc)) {
        prof_state(false);
        if (prof_pc.should_exit(pc)) {
            running = false;
            return;
        }
    }

    // TODO: DPI needs to handle this separately based on the simulation time
    mem->update_mtime();
    bool mstatus_MIE = (csr.at(CSR_MSTATUS).value & MSTATUS_MIE) >> 3;
    bool mie_MTIE = (csr.at(CSR_MIE).value & MIE_MTIE) >> 7;
    bool mip_MTIP = (csr.at(CSR_MIP).value & MIP_MTIP) >> 7;
    if (mstatus_MIE && mie_MTIE && mip_MTIP) {
        tu.e_timer_interrupt();
        #ifdef HW_MODELS_EN
        last_inst_branch = false;
        #endif
    }

    if (!tu.is_trapped()) {
        inst_fetch();
        #ifdef PROFILERS_EN
        prof.new_inst(inst);
        branch_taken = false;
        #endif

        uint32_t op_c = ip.copcode();
        if (op_c != 0x3) { // compressed ISA
            #ifdef RV32C
            INST_W(4);
            inst = inst & 0xffff;
            ip.inst = inst;
            switch (op_c) {
                case 0x0: c0(); break;
                case 0x1: c1(); break;
                case 0x2: c2(); break;
                default: tu.e_unsupported_inst("op_c unreachable");
            }
            #else // !RV32C
            tu.e_unsupported_inst("RV32C unsupported");
            #endif

        } else { // 32-bit ISA
            INST_W(8);
            switch (ip.opcode()) {
                CASE_DECODER(al_reg)
                CASE_DECODER(al_imm)
                CASE_DECODER(load)
                CASE_DECODER(store)
                CASE_DECODER(branch)
                CASE_DECODER(jalr)
                CASE_DECODER(jal)
                CASE_DECODER(lui)
                CASE_DECODER(auipc)
                CASE_DECODER(system)
                CASE_DECODER(misc_mem)
                CASE_DECODER(custom_ext)
                default: tu.e_unsupported_inst("opcode");
            }
        }
    }

    #ifdef PROFILERS_EN
    [[maybe_unused]] bool log_symbol = prof_perf.finish_inst(next_pc);
    #endif

    #if defined(PROFILERS_EN) || defined(HW_MODELS_EN)
    if (!tu.is_trapped() && (prof_act)) prof_pc.inst_cnt++;
    #endif

    #ifdef DASM_EN
    // dasm string always available, logged to the file conditionally
    DASM_ALIGN;
    dasm.finish_inst();
    if (logf.act) {
        if (tu.is_trapped()) {
            // log changed callstack and return
            log_ofstream << dasm.asm_str << "\n";
            if (log_symbol) LOG_SYMBOL_TO_FILE;
            if (cfg.exit_on_trap) running = false;
            return;
        }
        log_ofstream << INDENT << std::setw(6) << std::setfill(' ');
        // don't count instructions unless also profiling
        if (prof_act) log_ofstream << prof_pc.inst_cnt;
        else log_ofstream << "";
        log_ofstream << ": " << FORMAT_INST(pc, inst, inst_w) << " "
                     << dasm.asm_str;

        #ifdef HW_MODELS_EN
        if (cfg.log_hw_models) log_ofstream << hwmi.get_str();
        #endif
        log_ofstream << "\n";

        if (logf.state) {
            log_ofstream << print_state(csr_updated) << "\n";
            csr_updated = false;
        }
    }
    #endif
    if (tu.is_trapped()) {
        if (cfg.exit_on_trap) running = false;
        return;
    }

    #ifdef PROFILERS_EN
    prof.track_sp(rf[2]);
    if (prof_act && prof_trace) {
        prof.te.inst = inst;
        prof.te.pc = pc;
        prof.te.next_pc = next_pc;
        prof.te.sp = rf[2];
        prof.te.taken = branch_taken;
        prof.te.inst_size = TO_U8(inst_w >> 1); // hex digits to bytes
        #ifdef DPI
        prof.te.sample_cnt = clk_src.get_cr();
        #else
        prof.te.sample_cnt = prof_pc.inst_cnt;
        #endif
        #ifdef HW_MODELS_EN
        prof.te.ic_hm = TO_U8(hwrs.ic_hm);
        prof.te.dc_hm = TO_U8(hwrs.dc_hm);
        prof.te.bp_hm = TO_U8(hwrs.bp_hm);
        #endif
        prof.add_te();
    }
    #endif

    #ifdef DASM_EN
    if (log_symbol && logf.act && !prof_perf.dbg_check_top(next_pc)) {
        //log_ofstream << "would'be been mismatched callstack" << "\n";
        // risky but very likely to work since tail calls can be missed
        // TODO: should really be done on a callstack copy,
        // this will crash the sim if it pops all functions from callstack
        while (!prof_perf.dbg_check_top(next_pc)) prof_perf.dbg_pop_back();
    }
    if (log_symbol && logf.act) LOG_SYMBOL_TO_FILE;
    #endif

    pc = next_pc;
    inst_cnt++;

    if (inst_cnt == cfg.run_insts) running = false; // stop based on cli
}

// void core::reset() {
//     // TODO
// }

void core::finish(bool dump_regs) {
    if (dump_regs) dump();
    if ((cfg.mem_dump_start > 0) && (cfg.mem_dump_size > 0)) {
        mem->dump_as_words(cfg.mem_dump_start, cfg.mem_dump_size, cfg.out_dir);
    }
    #ifdef PROFILERS_EN
    prof_fusion.finish();
    prof_perf.finish();
    prof.finish();
    #endif
    #ifdef HW_MODELS_EN
    bp.finish(cfg.out_dir, prof_pc.inst_cnt);
    mem->cache_finish();
    log_hw_stats();
    #endif
    #ifdef DASM_EN
    log_ofstream << std::endl; // flush
    #endif
}

// Integer extension
void core::al_reg() {
    uint32_t funct7 = ip.funct7();
    uint32_t funct3 = ip.funct3();
    uint32_t alu_op_sel = ((ip.funct7_b5()) << 3) | funct3;
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
                default: tu.e_unsupported_inst("al_reg_rv32i"); return;
            }
            break;
        case 0x01: // rv32m
            switch (funct3) {
                CASE_ALU_REG_MUL_OP(mul)
                CASE_ALU_REG_MUL_OP(mulh)
                CASE_ALU_REG_MUL_OP(mulhsu)
                CASE_ALU_REG_MUL_OP(mulhu)
                CASE_ALU_REG_MUL_OP(div)
                CASE_ALU_REG_MUL_OP(divu)
                CASE_ALU_REG_MUL_OP(rem)
                CASE_ALU_REG_MUL_OP(remu)
                default: tu.e_unsupported_inst("al_reg_rv32m"); return;
            }
            break;
        case 0x05: // rv32 zbb
            switch (funct3) {
                CASE_ALU_REG_ZBB_OP(max)
                CASE_ALU_REG_ZBB_OP(maxu)
                CASE_ALU_REG_ZBB_OP(min)
                CASE_ALU_REG_ZBB_OP(minu)
                default: tu.e_unsupported_inst("al_reg_rv32_zbb"); return;
            }
            break;
        default: tu.e_unsupported_inst("al_reg"); return;
    }
    next_pc = pc + 4;
    #ifdef DASM_EN
    DASM_OP_RD << "," << DASM_OP_RS1 << "," << DASM_OP_RS2;
    DASM_RD_UPDATE;
    #endif
}

void core::al_imm() {
    uint32_t alu_op_sel_shift = ((ip.funct7_b5()) << 3) | ip.funct3();
    bool is_shift = (ip.funct3() & 0x3) == 1;
    uint32_t alu_op_sel = is_shift ? alu_op_sel_shift : ip.funct3();
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
        default: tu.e_unsupported_inst("al_imm"); return;
    }
    next_pc = pc + 4;
    #ifdef DASM_EN
    DASM_OP_RD << "," << DASM_OP_RS1 << ",";
    if (is_shift) dasm.asm_ss << FHEXN(ip.imm_i_shamt(), 2);
    else dasm.asm_ss << TO_I32(ip.imm_i());
    DASM_RD_UPDATE;
    if (inst == INST_NOP) {
        dasm.clear_str();
        dasm.asm_ss << "nop";
    }
    #endif
}

void core::load() {
    #ifdef PROFILERS_EN
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

void core::store() {
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

void core::branch() {
    [[maybe_unused]] bool taken; // unused if built without PROFILERS_EN
    next_pc = pc + 4; // updated in CASE_BRANCH if taken
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
    prof_perf.set_perf_event_flag(perf_event_t::branches);
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
    if (!no_bp) { // not accessing icache in case there is no bp, stall instead
        //mem->speculative_exec(speculative_t::enter);
        inst_speculative = mem->rd_inst(speculative_next_pc);
    }

    next_ic_hm = hwrs.ic_hm; // count speculative access, not the resolution
    bp.update(pc, next_pc);
    bool correct = (next_pc == speculative_next_pc);
    if (!correct) {
        //mem->speculative_exec(speculative_t::exit_flush);
        inst_resolved = mem->rd_inst(next_pc);
        #ifdef PROFILERS_EN
        prof_perf.set_perf_event_flag(perf_event_t::bp_mispredict);
        #endif
    } else {
        //mem->speculative_exec(speculative_t::exit_commit);
        inst_resolved = inst_speculative;
    }

    hwrs.bp_hm = static_cast<hw_status_t>(correct);
    //next_ic_hm = hwrs.ic_hm;
    hwrs.ic_hm = save_ic_hm;

    #ifdef DASM_EN
    hwmi.log_bp({bp_name, b_dir_t::forward, correct, taken});
    #endif

    #endif // HW_MODELS_EN
}

void core::jalr() {
    if (ip.funct3() != 0) tu.e_unsupported_inst("jalr, funct3 != 0");
    next_pc = (rf[ip.rs1()] + ip.imm_i()) & 0xFFFFFFFE;

    #ifndef RV32C
    bool address_unaligned = (next_pc % 4 != 0);
    if (address_unaligned) {
        tu.e_inst_addr_misaligned(next_pc, "jalr unaligned access");
        return;
    }
    #endif

    uint32_t ra = pc + 4;
    write_rf(ip.rd(), ra);
    bool ret_inst = (inst == INST_RET) || (inst == INST_RET_X5); // but not X15
    if (ret_inst) { DASM_OP(ret) }
    else { DASM_OP(jalr) }
    PROF_J(jalr)
    PROF_RD_RS1

    #ifdef PROFILERS_EN
    bool tail_call = (ip.rd() == 0);
    prof_perf.update_jalr(next_pc, ret_inst, tail_call, ra);
    branch_taken = true;
    #endif

    #ifdef DASM_EN
    if (ret_inst) {
        dasm.asm_ss << dasm.op;
    } else {
        DASM_OP_RD << "," << TO_I32(ip.imm_i()) << "(" << DASM_OP_RS1 << ")";
    }
    DASM_RD_UPDATE;
    #endif
}

void core::jal() {
    uint32_t ra = pc + 4;
    write_rf(ip.rd(), ra);
    next_pc = pc + ip.imm_j();

    #ifndef RV32C
    bool address_unaligned = (next_pc % 4 != 0);
    if (address_unaligned) {
        tu.e_inst_addr_misaligned(next_pc, "jal unaligned access");
        return;
    }
    #endif

    DASM_OP(jal)
    PROF_J(jal)
    PROF_RD

    #ifdef PROFILERS_EN
    //bool pc_match = (pc == 0x19118); // known noreturn call
    bool tail_call = (ip.rd() == 0); // || pc_match;
    prof_perf.update_jal(next_pc, tail_call, ra);
    branch_taken = true;
    #endif

    #ifdef DASM_EN
    DASM_OP_RD << "," << std::hex << pc + TO_I32(ip.imm_j()) << std::dec;
    DASM_RD_UPDATE;
    #endif
}

void core::lui() {
    write_rf(ip.rd(), ip.imm_u());
    next_pc = pc + 4;
    DASM_OP(lui)
    PROF_G(lui)
    PROF_RD
    #ifdef DASM_EN
    DASM_OP_RD << ", " << FHEXN((ip.imm_u() >> 12), 5);
    DASM_RD_UPDATE;
    #endif
}

void core::auipc() {
    write_rf(ip.rd(), ip.imm_u() + pc);
    next_pc = pc + 4;
    DASM_OP(auipc)
    PROF_G(auipc)
    PROF_RD
    #ifdef DASM_EN
    DASM_OP_RD << ", " << FHEXN((ip.imm_u() >> 12), 5);
    DASM_RD_UPDATE;
    #endif
}

void core::system() {
    uint32_t funct3 = ip.funct3();
    if (funct3) {
        csr_access();
        next_pc = pc + 4;
    } else { // (funct3 == 0) -> system instructions
        switch (inst) {
            case INST_ECALL:
                tu.e_env("ECALL", MCAUSE_MACHINE_ECALL);
                return;
            case INST_EBREAK:
                tu.e_env("EBREAK", MCAUSE_BREAKPOINT);
                return;
            case INST_MRET:
                // restore previous interrupt enable bit state
                csr.at(CSR_MSTATUS).value =
                    (csr.at(CSR_MSTATUS).value & ~MSTATUS_MIE) |
                    ((csr.at(CSR_MSTATUS).value & MSTATUS_MPIE) >> 4);
                // restore pc
                next_pc = csr[CSR_MEPC].value;
                DASM_OP(mret)
                PROF_G(mret)
                #ifdef PROFILERS_EN
                prof_perf.update_jalr(next_pc, true, false, 0u);
                #endif
                break;
            default: tu.e_unsupported_inst("system");
        }
        #ifdef DASM_EN
        dasm.asm_ss << dasm.op;
        #endif
    }
}

void core::misc_mem() {
    if (inst == INST_FENCE_I) {
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

// Custom extension
void core::custom_ext() {
    uint8_t funct3 = ip.funct3();
    uint8_t funct7 = ip.funct7();
    if (funct3 == TO_U8(custom_ext_t::arith)) {
        switch (funct7) {
            CASE_ALU_CUSTOM_OP(add16)
            CASE_ALU_CUSTOM_OP(add8)
            CASE_ALU_CUSTOM_OP(sub16)
            CASE_ALU_CUSTOM_OP(sub8)
            CASE_ALU_CUSTOM_OP_PAIR(mul16)
            CASE_ALU_CUSTOM_OP_PAIR(mul16u)
            CASE_ALU_CUSTOM_OP_PAIR(mul8)
            CASE_ALU_CUSTOM_OP_PAIR(mul8u)
            CASE_ALU_CUSTOM_OP(dot16)
            CASE_ALU_CUSTOM_OP(dot8)
            CASE_ALU_CUSTOM_OP(dot4)
            default : tu.e_unsupported_inst("custom extension - arith");
        }

        #ifdef PROFILERS_EN
        prof_perf.set_perf_event_flag(perf_event_t::simd);
        #endif

        #ifdef DASM_EN
        DASM_OP_RD << "," << DASM_OP_RS1 << "," << DASM_OP_RS2;
        DASM_RD_UPDATE;
        bool paired_arith = (funct7 == TO_U8(alu_custom_op_t::op_mul16) ||
                             funct7 == TO_U8(alu_custom_op_t::op_mul16u) ||
                             funct7 == TO_U8(alu_custom_op_t::op_mul8) ||
                             funct7 == TO_U8(alu_custom_op_t::op_mul8u));
        if (paired_arith) DASM_RD_UPDATE_PAIR;
        #endif

    } else if (funct3 == TO_U8(custom_ext_t::memory)) {
        uint32_t rs1 = rf[ip.rs1()];
        switch (funct7) {
            CASE_MEM_CUSTOM_OP(unpk16)
            CASE_MEM_CUSTOM_OP(unpk16u)
            CASE_MEM_CUSTOM_OP(unpk8)
            CASE_MEM_CUSTOM_OP(unpk8u)
            CASE_MEM_CUSTOM_OP(unpk4)
            CASE_MEM_CUSTOM_OP(unpk4u)
            CASE_MEM_CUSTOM_OP(unpk2)
            CASE_MEM_CUSTOM_OP(unpk2u)
            default: tu.e_unsupported_inst("custom extension - memory");
        }

        #ifdef PROFILERS_EN
        prof_perf.set_perf_event_flag(perf_event_t::simd);
        #endif

        #ifdef DASM_EN
        DASM_OP_RD << "," << DASM_OP_RS1;
        DASM_RD_UPDATE;
        DASM_RD_UPDATE_PAIR;
        #endif

    } else if (funct3 == TO_U8(custom_ext_t::hints)) {
        switch (funct7) {
            CASE_SCP_CUSTOM(lcl); DASM_OP(scp.lcl); break;
            CASE_SCP_CUSTOM(rel); DASM_OP(scp.rel); break;
            default : tu.e_unsupported_inst("custom extension - hint");
        }

        #ifdef DASM_EN
        DASM_OP_RD << "," << DASM_OP_RS1;
        DASM_RD_UPDATE;
        #endif

    } else {
        tu.e_unsupported_inst("custom extension");
    }
    next_pc = pc + 4;
}

uint32_t core::al_c_add16(uint32_t a, uint32_t b) {
    // parallel add 2 halfword chunks
    constexpr size_t e = 2;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t sum = TO_I32(TO_I16(a & 0xffff)) + TO_I32(TO_I16(b & 0xffff));
        #ifdef DASM_EN
        simd_ss_append(
            TO_I32(TO_I16(sum & 0xFFFF)),
            TO_I32(TO_I16(a & 0xffff)),
            TO_I32(TO_I16(b & 0xffff))
        );
        #endif
        a >>= 16;
        b >>= 16;
        res |= (sum & 0xFFFF) << (i * 16);
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", "]");
    #endif

    return res;
}

uint32_t core::al_c_add8(uint32_t a, uint32_t b) {
    // parallel add 4 byte chunks
    constexpr size_t e = 4;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t sum = TO_I32(TO_I8(a & 0xff)) + TO_I32(TO_I8(b & 0xff));
        #ifdef DASM_EN
        simd_ss_append(
            TO_I32(TO_I8(sum & 0xff)),
            TO_I32(TO_I8(a & 0xff)),
            TO_I32(TO_I8(b & 0xff))
        );
        #endif
        a >>= 8;
        b >>= 8;
        res |= (sum & 0xFF) << (i * 8);
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", "]");
    #endif

    return res;
}

uint32_t core::al_c_sub16(uint32_t a, uint32_t b) {
    // parallel sub 2 halfword chunks
    constexpr size_t e = 2;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t sum = TO_I32(TO_I16(a & 0xffff)) - TO_I32(TO_I16(b & 0xffff));
        #ifdef DASM_EN
        simd_ss_append(
            TO_I32(TO_I16(sum & 0xFFFF)),
            TO_I32(TO_I16(a & 0xffff)),
            TO_I32(TO_I16(b & 0xffff))
        );
        #endif
        a >>= 16;
        b >>= 16;
        res |= (sum & 0xFFFF) << (i * 16);
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", "]");
    #endif

    return res;
}

uint32_t core::al_c_sub8(uint32_t a, uint32_t b) {
    // parallel sub 4 byte chunks
    constexpr size_t e = 4;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        int32_t sum = TO_I32(TO_I8(a & 0xff)) - TO_I32(TO_I8(b & 0xff));
        #ifdef DASM_EN
        simd_ss_append(
            TO_I32(TO_I8(sum & 0xff)),
            TO_I32(TO_I8(a & 0xff)),
            TO_I32(TO_I8(b & 0xff))
        );
        #endif
        a >>= 8;
        b >>= 8;
        res |= (sum & 0xFF) << (i * 8);
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", "]");
    #endif

    return res;
}

reg_pair core::al_c_mul16(uint32_t a, uint32_t b) {
    // multiply 2 halfword chunks into 2 32-bit results
    int32_t words[2];
    #ifdef DASM_EN
    simd_ss_init("", "[ ", "[ ");
    #endif

    for (auto &word : words) {
        word = TO_I32(TO_I16(a & 0xffff)) * TO_I32(TO_I16(b & 0xffff));
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I16(a & 0xffff)), TO_I32(TO_I16(b & 0xffff)));
        #endif
        a >>= 16;
        b >>= 16;
    }

    #ifdef DASM_EN
    dasm.simd_c << TO_I32(words[0]) << ", " << TO_I32(words[1]);
    simd_ss_finish("", "]", "]");
    #endif

    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::al_c_mul16u(uint32_t a, uint32_t b) {
    // multiply 2 halfword chunks into 2 32-bit results
    uint32_t words[2];
    #ifdef DASM_EN
    simd_ss_init("", "[ ", "[ ");
    #endif

    for (auto &word : words) {
        word = TO_U32(TO_U16(a & 0xffff)) * TO_U32(TO_U16(b & 0xffff));
        #ifdef DASM_EN
        simd_ss_append(TO_U32(TO_U16(a & 0xffff)), TO_U32(TO_U16(b & 0xffff)));
        #endif
        a >>= 16;
        b >>= 16;
    }

    #ifdef DASM_EN
    dasm.simd_c << TO_U32(words[0]) << ", " << TO_U32(words[1]);
    simd_ss_finish("", "]", "]");
    #endif

    return {words[0], words[1]};
}

reg_pair core::al_c_mul8(uint32_t a, uint32_t b) {
    // multiply 4 byte chunks into 2 32-bit results
    int16_t halves[4];
    #ifdef DASM_EN
    simd_ss_init("", "[ ", "[ ");
    #endif

    for (auto &half : halves) {
        half = TO_I16(TO_I8(a & 0xff)) * TO_I16(TO_I8(b & 0xff));
        #ifdef DASM_EN
        simd_ss_append(TO_I16(TO_I8(a & 0xff)), TO_I16(TO_I8(b & 0xff)));
        #endif
        a >>= 8;
        b >>= 8;
    }

    #ifdef DASM_EN
    dasm.simd_c << "[ " << TO_I32(TO_I16(halves[0]) & 0xFFFF) << " "
                << TO_I32(TO_I16(halves[1]) & 0xFFFF) << " ], "
                << "[ " << TO_I32(TO_I16(halves[2]) & 0xFFFF) << " "
                << TO_I32(TO_I16(halves[3]) & 0xFFFF) << " ]";
    simd_ss_finish("", "]", "]");
    #endif

    int32_t words[2] = {0, 0};
    words[0] = (TO_I32(halves[0]) & 0xFFFF) |
               ((TO_I32(halves[1]) & 0xFFFF) << 16);
    words[1] = (TO_I32(halves[2]) & 0xFFFF) |
               ((TO_I32(halves[3]) & 0xFFFF) << 16);
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::al_c_mul8u(uint32_t a, uint32_t b) {
    // multiply 4 byte chunks into 2 32-bit results
    uint16_t halves[4];
    #ifdef DASM_EN
    simd_ss_init("", "[ ", "[ ");
    #endif

    for (auto &half : halves) {
        half = TO_U16(TO_U8(a & 0xff)) * TO_U16(TO_U8(b & 0xff));
        #ifdef DASM_EN
        simd_ss_append(TO_U16(TO_U8(a & 0xff)), TO_U16(TO_U8(b & 0xff)));
        #endif
        a >>= 8;
        b >>= 8;
    }

    #ifdef DASM_EN
    dasm.simd_c << "[ " << TO_U32(TO_U16(halves[0]) & 0xFFFF) << " "
                << TO_U32(TO_U16(halves[1]) & 0xFFFF) << " ], "
                << "[ " << TO_U32(TO_U16(halves[2]) & 0xFFFF) << " "
                << TO_U32(TO_U16(halves[3]) & 0xFFFF) << " ]";
    simd_ss_finish("", "]", "]");
    #endif

    uint32_t words[2] = {0, 0};
    words[0] = (TO_U32(halves[0]) & 0xFFFF) |
               ((TO_U32(halves[1]) & 0xFFFF) << 16);
    words[1] = (TO_U32(halves[2]) & 0xFFFF) |
               ((TO_U32(halves[3]) & 0xFFFF) << 16);
    return {words[0], words[1]};
}

uint32_t core::al_c_dot16(uint32_t a, uint32_t b) {
    // multiply 2 halfword chunks and sum the results
    constexpr size_t e = 2;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I16(a & 0xffff)), TO_I32(TO_I16(b & 0xffff)));
        #endif
        res += TO_I32(TO_I16(a & 0xffff)) * TO_I32(TO_I16(b & 0xffff));
        a >>= 16;
        b >>= 16;
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", res);
    #endif

    #ifdef PROFILERS_EN
    prof.log_sparsity(res==0);
    #endif

    return res;
}

uint32_t core::al_c_dot8(uint32_t a, uint32_t b) {
    // multiply 4 byte chunks and sum the results
    constexpr size_t e = 4;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I8(a & 0xff)), TO_I32(TO_I8(b & 0xff)));
        #endif
        res += TO_I32(TO_I8(a & 0xff)) * TO_I32(TO_I8(b & 0xff));
        a >>= 8;
        b >>= 8;
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", res);
    #endif

    #ifdef PROFILERS_EN
    prof.log_sparsity(res==0);
    #endif

    return res;
}

uint32_t core::al_c_dot4(uint32_t a, uint32_t b) {
    // multiply 8 nibble chunks and sum the results
    constexpr size_t e = 8;
    int32_t res = 0;
    #ifdef DASM_EN
    simd_ss_init("[ ", "[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I4(a & 0xf)), TO_I32(TO_I4(b & 0xf)));
        #endif
        res += TO_I32(TO_I4(a & 0xf)) * TO_I32(TO_I4(b & 0xf));
        a >>= 4;
        b >>= 4;
    }

    #ifdef DASM_EN
    simd_ss_finish("]", "]", res);
    #endif
    #ifdef PROFILERS_EN
    prof.log_sparsity(res==0);
    #endif

    return res;
}

reg_pair core::mem_c_unpk16(uint32_t a) {
    // unpack 2 16-bit values to 2 32-bit values
    constexpr size_t e = 2;
    int16_t halves[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        halves[i] = TO_I16(a & 0xffff);
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I16(a & 0xffff)));
        #endif
        a >>= 16;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << TO_I32(halves[0]) << ", " << TO_I32(halves[1]);
    simd_ss_finish("");
    #endif

    return {TO_U32(halves[0]), TO_U32(halves[1])};
}

reg_pair core::mem_c_unpk16u(uint32_t a) {
    // unpack 2 16-bit values to 2 32-bit values unsigned
    constexpr size_t e = 2;
    uint16_t halves[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        halves[i] = TO_U16(a & 0xffff);
        #ifdef DASM_EN
        simd_ss_append(TO_U32(TO_U16(a & 0xffff)));
        #endif
        a >>= 16;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << TO_U32(TO_U16(halves[0])) << ", "
                << TO_U32(TO_U16(halves[1]));
    simd_ss_finish("");
    #endif

    return {TO_U32(halves[0]), TO_U32(halves[1])};
}

reg_pair core::mem_c_unpk8(uint32_t a) {
    // unpack 4 8-bit values to 4 16-bit values (as 2 32-bit values)
    constexpr size_t e = 4;
    int8_t bytes[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        bytes[i] = TO_I8(a & 0xff);
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I8(a & 0xff)));
        #endif
        a >>= 8;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ " << TO_I32(TO_I16(bytes[0])) << " "
                << TO_I32(TO_I16(bytes[1])) << " ], "
                << "[ " << TO_I32(TO_I16(bytes[2])) << " "
                << TO_I32(TO_I16(bytes[3])) << " ]";
    simd_ss_finish("");
    #endif

    int32_t words[2];
    words[0] = (TO_I32(TO_I16(bytes[0])) & 0xFFFF) |
               ((TO_I32(TO_I16(bytes[1])) & 0xFFFF) << 16);
    words[1] = (TO_I32(TO_I16(bytes[2])) & 0xFFFF) |
               ((TO_I32(TO_I16(bytes[3])) & 0xFFFF) << 16);
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::mem_c_unpk8u(uint32_t a) {
    // unpack 4 8-bit values to 4 16-bit values (as 2 32-bit values) unsigned
    constexpr size_t e = 4;
    uint8_t bytes[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        bytes[i] = TO_U8(a & 0xff);
        #ifdef DASM_EN
        simd_ss_append(TO_U32(TO_U8(a & 0xff)));
        #endif
        a >>= 8;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ " << TO_U32(TO_U16(bytes[0])) << " "
                << TO_U32(TO_U16(bytes[1])) << " ], "
                << "[ " << TO_U32(TO_U16(bytes[2])) << " "
                << TO_U32(TO_U16(bytes[3])) << " ]";
    simd_ss_finish("");
    #endif

    uint32_t words[2];
    words[0] = (TO_U32(TO_U16(bytes[0])) & 0xFFFF) |
               ((TO_U32(TO_U16(bytes[1])) & 0xFFFF) << 16);
    words[1] = (TO_U32(TO_U16(bytes[2])) & 0xFFFF) |
               ((TO_U32(TO_U16(bytes[3])) & 0xFFFF) << 16);
    return {words[0], words[1]};
}

reg_pair core::mem_c_unpk4(uint32_t a) {
    // unpack 8 4-bit values to 8 8-bit values (as 2 32-bit values)
    constexpr size_t e = 8;
    int8_t nibbles[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        nibbles[i] = TO_I4(a & 0xf);
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I4(a & 0xf)));
        #endif
        a >>= 4;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ ";
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << TO_I32(TO_I8(nibbles[i])) << " ";
        if (i==3) dasm.simd_c << "], [ ";
        if (i==7) dasm.simd_c << "]";
    }
    simd_ss_finish("");
    #endif

    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < (e>>1); i++) {
        words[0] |= (TO_I32(TO_I8(nibbles[i])) & 0xFF) << (i * 8);
        words[1] |= (TO_I32(TO_I8(nibbles[i + 4])) & 0xFF) << (i * 8);
    }
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::mem_c_unpk4u(uint32_t a) {
    // unpack 8 4-bit values to 8 8-bit values (as 2 32-bit values) unsigned
    constexpr size_t e = 8;
    uint8_t nibbles[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        nibbles[i] = TO_U4(a & 0xf);
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_U4(a & 0xf)));
        #endif
        a >>= 4;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ ";
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << TO_U32(TO_U8(nibbles[i])) << " ";
        if (i==3) dasm.simd_c << "], [ ";
        if (i==7) dasm.simd_c << "]";
    }
    simd_ss_finish("");
    #endif

    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < (e>>1); i++) {
        words[0] |= (TO_U32(TO_U8(nibbles[i])) & 0xFF) << (i * 8);
        words[1] |= (TO_U32(TO_U8(nibbles[i + 4])) & 0xFF) << (i * 8);
    }
    return {words[0], words[1]};
}

reg_pair core::mem_c_unpk2(uint32_t a) {
    // unpack 16 2-bit values to 16 4-bit values (as 2 32-bit values)
    constexpr size_t e = 16;
    int8_t crumbs[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        crumbs[i] = TO_I2(a & 0x3);
        #ifdef DASM_EN
        simd_ss_append(TO_I32(TO_I2(a & 0x3)));
        #endif
        a >>= 2;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ ";
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << TO_I32(TO_I4(crumbs[i])) << " ";
        if (i==7) dasm.simd_c << "], [ ";
        if (i==15) dasm.simd_c << "]";
    }
    simd_ss_finish("");
    #endif

    int32_t words[2] = {0, 0};
    for (size_t i = 0; i < (e>>1); i++) {
        words[0] |= (TO_I32(TO_I8(crumbs[i])) & 0xF) << (i * 4);
        words[1] |= (TO_I32(TO_I8(crumbs[i + 8])) & 0xF) << (i * 4);
    }
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::mem_c_unpk2u(uint32_t a) {
    // unpack 16 2-bit values to 16 4-bit values (as 2 32-bit values) unsigned
    constexpr size_t e = 16;
    uint8_t crumbs[e];
    #ifdef DASM_EN
    simd_ss_init("[ ");
    #endif

    for (size_t i = 0; i < e; i++) {
        crumbs[i] = TO_U2(a & 0x3);
        #ifdef DASM_EN
        simd_ss_append(TO_U32(TO_U2(a & 0x3)));
        #endif
        a >>= 2;
    }

    #ifdef DASM_EN
    dasm.simd_a << "] ";
    dasm.simd_c << "[ ";
    for (size_t i = 0; i < e; i++) {
        dasm.simd_c << TO_U32(TO_U4(crumbs[i])) << " ";
        if (i==7) dasm.simd_c << "], [ ";
        if (i==15) dasm.simd_c << "]";
    }
    simd_ss_finish("");
    #endif

    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < (e>>1); i++) {
        words[0] |= (TO_U32(TO_U8(crumbs[i])) & 0xF) << (i * 4);
        words[1] |= (TO_U32(TO_U8(crumbs[i+8])) & 0xF) << (i * 4);
    }
    return {words[0], words[1]};
}

#ifdef DASM_EN
void core::simd_ss_init(std::string a) {
    dasm.simd_a.str("");
    dasm.simd_c.str("");
    dasm.simd_a << a;
}

void core::simd_ss_init(std::string a, std::string b) {
    dasm.simd_a.str("");
    dasm.simd_b.str("");
    dasm.simd_a << a;
    dasm.simd_b << b;
}

void core::simd_ss_init(std::string c, std::string a, std::string b) {
    simd_ss_init(a, b);
    dasm.simd_c.str("");
    dasm.simd_c << c;
}

void core::simd_ss_append(int32_t a) {
    dasm.simd_a << a << " ";
}

void core::simd_ss_append(int32_t a, int32_t b) {
    dasm.simd_a << a << " ";
    dasm.simd_b << b << " ";
}

void core::simd_ss_append(int32_t c, int32_t a, int32_t b) {
    simd_ss_append(a, b);
    dasm.simd_c << c << " ";
}

void core::simd_ss_finish(std::string a) {
    dasm.simd_a << a;
    dasm.simd_ss << "; RD = " << dasm.simd_c.str()
                 << ", RS1 = " << dasm.simd_a.str();
}

void core::simd_ss_finish(std::string a, std::string b, int32_t res) {
    dasm.simd_a << a;
    dasm.simd_b << b;
    dasm.simd_ss << "; RD = " << res
                 << ", RS1 = " << dasm.simd_a.str()
                 << ", RS2 = " << dasm.simd_b.str();
}

void core::simd_ss_finish(std::string c, std::string a, std::string b) {
    dasm.simd_a << a;
    dasm.simd_b << b;
    dasm.simd_c << c;
    dasm.simd_ss << "; RD = " << dasm.simd_c.str()
                 << ", RS1 = " << dasm.simd_a.str()
                 << ", RS2 = " << dasm.simd_b.str();
}
#endif

// Zicsr extension
void core::csr_access() {
    cntr_update();
    uint16_t csr_addr = TO_U16(ip.csr_addr());
    auto it = csr.find(csr_addr);
    if (it == csr.end()) {
        #ifdef DASM_EN
        DASM_TRAP << "Unsupported CSR. Address: " << FHEXN(csr_addr, 3);
        #endif
        SIM_TRAP << "Unsupported CSR. Address: " << FHEXN(csr_addr, 3) << "\n";
    } else {
        // using temp in case rd and rs1 are the same register
        uint32_t init_val_rs1 = rf[ip.rs1()];
        // FIXME: rw/rwi should not read CSR on rd=x0; no impact w/ current CSRs
        write_rf(ip.rd(), it->second.value);
        switch (ip.funct3()) {
            CASE_CSR(rw)
            CASE_CSR(rs)
            CASE_CSR(rc)
            CASE_CSR_I(rwi)
            CASE_CSR_I(rsi)
            CASE_CSR_I(rci)
            default: tu.e_unsupported_inst("sys");
        }
        if (csr.at(CSR_TOHOST).value & 0x1) running = false;
    }

    #ifdef DASM_EN
    bool imm_type = (ip.funct3() & 0x4);
    DASM_RD_UPDATE;
    if (ip.rs1() || imm_type) {
        if (ip.rd()) dasm.asm_ss << "; ";
        DASM_ALIGN;
        dasm.asm_ss << CSRF(it);
    }
    if (logf.state) csr_updated = true;
    #endif
}

// Zicntr extension
void core::cntr_update() {
    // TODO: for DPI environment, these have to either be revisited or ignored
    uint64_t inst_elapsed = inst_cnt - inst_cnt_csr;
    csr.at(CSR_MCYCLE).value += inst_elapsed & 0xFFFFFFFF;
    csr.at(CSR_MCYCLEH).value += (inst_elapsed >> 32) & 0xFFFFFFFF;
    csr.at(CSR_MINSTRET).value += inst_elapsed & 0xFFFFFFFF;
    csr.at(CSR_MINSTRETH).value += (inst_elapsed >> 32) & 0xFFFFFFFF;
    inst_cnt_csr = inst_cnt;

    // user mode shadows
    csr.at(CSR_CYCLE).value = csr.at(CSR_MCYCLE).value;
    csr.at(CSR_CYCLEH).value = csr.at(CSR_MCYCLEH).value;
    csr.at(CSR_INSTRET).value = csr.at(CSR_MINSTRET).value;
    csr.at(CSR_INSTRETH).value = csr.at(CSR_MINSTRETH).value;
    uint64_t mtime_shadow = mem->get_mtime_shadow();
    csr.at(CSR_TIME).value = TO_U32(mtime_shadow);
    csr.at(CSR_TIMEH).value = TO_U32(mtime_shadow >> 32);
}

// C extension - decoders
void core::c0() {
    uint32_t funct3 = ip.cfunct3();
    switch (funct3) {
        case 0x0: c_addi4spn(); break;
        case 0x2: c_lw(); break;
        case 0x6: c_sw(); break;
        default: tu.e_unsupported_inst("c0");
    }
}

void core::c1() {
    uint32_t funct3 = ip.cfunct3();
    uint32_t funct2h = ip.cfunct2h();
    uint32_t funct2l = ip.cfunct2l();
    uint32_t funct6 = ip.cfunct6();

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
                        default: tu.e_unsupported_inst("c1:0x4:0x3");
                    }
                    break;
            }
            break;
        case 0x5: c_j(); break;
        case 0x6: c_beqz(); break;
        case 0x7: c_bnez(); break;
        default: tu.e_unsupported_inst("c1");
    }
}

void core::c2() {
    uint32_t funct4 = ip.cfunct4();
    uint32_t funct3 = ip.cfunct3();
    switch (funct3) {
        case 0x0: c_slli(); break;
        case 0x2: c_lwsp(); break;
        case 0x6: c_swsp(); break;
        case 0x4:
            switch (funct4) {
                case 0x8:
                    if (ip.crs2() == 0x0) c_jr();
                    else c_mv();
                    break;
                case 0x9:
                    if (ip.crs2() == 0x0 && ip.rd() == 0x0) c_ebreak();
                    else if (ip.crs2() == 0x0) c_jalr();
                    else c_add();
                break;
                default: tu.e_unsupported_inst("unreachable");
            }
            break;
        default: tu.e_unsupported_inst("c2");
    }
}

// C extension - arithmetic and logic operations
void core::c_addi() {
    write_rf(ip.rd(), al_addi(rf[ip.rd()], ip.imm_c_arith()));
    DASM_OP(c.addi)
    PROF_G(c_addi)
    #ifdef DASM_EN
    DASM_OP_RD << "," << TO_I32(ip.imm_c_arith());
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_li() {
    write_rf(ip.rd(), ip.imm_c_arith());
    DASM_OP(c.li)
    PROF_G(c_li)
    #ifdef DASM_EN
    DASM_OP_RD << "," << TO_I32(ip.imm_c_arith());
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_lui() {
    if (ip.imm_c_lui() == 0) tu.e_illegal_inst("c.lui (imm=0)", 4);
    write_rf(ip.rd(), ip.imm_c_lui());
    DASM_OP(c.lui)
    PROF_G(c_lui)
    #ifdef DASM_EN
    DASM_OP_RD << "," << FHEXN((ip.imm_c_lui() >> 12), 5);
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_nop() {
    // write_rf(0, al_addi(rf[0], 0));
    DASM_OP(c.nop)
    PROF_G(c_nop)
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op;
    #endif
    next_pc = pc + 2;
}

void core::c_addi16sp() {
    if (ip.imm_c_16sp() == 0) tu.e_illegal_inst("c.addi16sp (imm=0)", 4);
    write_rf(2, al_addi(rf[2], ip.imm_c_16sp()));
    DASM_OP(c.addi16sp)
    PROF_G(c_addi16sp)
    #ifdef DASM_EN
    DASM_OP_RD << "," << TO_I32(ip.imm_c_16sp());
    DASM_RD_UPDATE_P(2);
    #endif
    next_pc = pc + 2;
}

void core::c_srli() {
    write_rf(ip.cregh(), al_srli(rf[ip.cregh()], ip.imm_c_arith()));
    DASM_OP(c.srli)
    PROF_G(c_srli)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << TO_I32(ip.imm_c_arith());
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_srai() {
    write_rf(ip.cregh(), al_srai(rf[ip.cregh()], ip.imm_c_arith()));
    DASM_OP(c.srai)
    PROF_G(c_srai)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << TO_I32(ip.imm_c_arith());
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_andi() {
    write_rf(ip.cregh(), al_andi(rf[ip.cregh()], ip.imm_c_arith()));
    DASM_OP(c.andi)
    PROF_G(c_andi)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << TO_I32(ip.imm_c_arith());
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_and() {
    write_rf(ip.cregh(), al_and(rf[ip.cregh()], rf[ip.cregl()]));
    DASM_OP(c.and)
    PROF_G(c_and)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_or() {
    write_rf(ip.cregh(), al_or(rf[ip.cregh()], rf[ip.cregl()]));
    DASM_OP(c.or)
    PROF_G(c_or)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_xor() {
    write_rf(ip.cregh(), al_xor(rf[ip.cregh()], rf[ip.cregl()]));
    DASM_OP(c.xor)
    PROF_G(c_xor)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_sub() {
    write_rf(ip.cregh(), al_sub(rf[ip.cregh()], rf[ip.cregl()]));
    DASM_OP(c.sub)
    PROF_G(c_sub)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_addi4spn() {
    if (ip.imm_c_4spn() == 0) tu.e_illegal_inst("c.addi4spn (imm=0)", 4);
    if (inst == 0) tu.e_illegal_inst("c.inst == 0", 4);
    write_rf(ip.cregl(), al_addi(rf[2], ip.imm_c_4spn()));
    DASM_OP(c.addi4spn)
    PROF_G(c_addi4spn)
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << DASM_CREGL << ",x2,"
                << TO_I32(ip.imm_c_4spn());
    DASM_RD_UPDATE_P(ip.cregl());
    #endif
    next_pc = pc + 2;
}

void core::c_slli() {
    write_rf(ip.rd(), al_sll(rf[ip.rd()], ip.imm_c_slli()));
    DASM_OP(c.slli)
    PROF_G(c_slli)
    #ifdef PROFILERS_EN
    prof_fusion.attack({trigger::slli_lea, inst, mem->just_inst(pc + 2), true});
    #endif
    #ifdef DASM_EN
    DASM_OP_RD << "," << FHEXN(TO_I32(ip.imm_c_slli()), 2);
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_mv() {
    write_rf(ip.rd(), rf[ip.crs2()]);
    DASM_OP(c.mv)
    PROF_G(c_mv)
    #ifdef DASM_EN
    DASM_OP_RD << "," << rf_names[ip.crs2()][rf_names_idx];
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_add() {
    write_rf(ip.rd(), al_add(rf[ip.rd()], rf[ip.crs2()]));
    DASM_OP(c.add)
    PROF_G(c_add)
    #ifdef DASM_EN
    DASM_OP_RD << "," << rf_names[ip.crs2()][rf_names_idx];
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

// C extension - memory operations
void core::c_lw() {
    uint32_t rs1 = rf[ip.cregh()];
    uint32_t loaded = mem->rd(rs1 + ip.imm_c_mem(), 4u);
    if (tu.is_trapped()) return;
    write_rf(ip.cregl(), loaded);
    DASM_OP(c.lw)
    PROF_G(c_lw)
    #ifdef PROFILERS_EN
    prof.log_stack_access_load((rs1 + ip.imm_c_mem()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << DASM_CREGL << "," << TO_I32(ip.imm_c_mem())
                << "(" << rf_names[ip.cregh()][rf_names_idx] << ")";
    DASM_RD_UPDATE_P(ip.cregl());
    if (ip.rd()) {
        dasm.asm_ss << " <- mem["
                    << MEM_ADDR_FORMAT(TO_I32(ip.imm_c_mem()) + rs1) << "]";
    }
    #endif
    next_pc = pc + 2;
}

void core::c_lwsp() {
    uint32_t loaded = mem->rd(rf[2] + ip.imm_c_lwsp(), 4u);
    if (tu.is_trapped()) return;
    write_rf(ip.rd(), loaded);
    DASM_OP(c.lwsp)
    PROF_G(c_lwsp)
    #ifdef PROFILERS_EN
    prof.log_stack_access_load((rf[2] + ip.imm_c_lwsp()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef DASM_EN
    DASM_OP_RD << "," << TO_I32(ip.imm_c_lwsp())
               << "(" << rf_names[2][rf_names_idx] << ")";
    DASM_RD_UPDATE;
    if (ip.rd()) {
        dasm.asm_ss << " <- mem["
                    << MEM_ADDR_FORMAT(TO_I32(ip.imm_c_lwsp()) + rf[2]) << "]";
    }
    #endif
    next_pc = pc + 2;
}

void core::c_sw() {
    mem->wr(rf[ip.cregh()] + ip.imm_c_mem(), rf[ip.cregl()], 4u);
    if (tu.is_trapped()) return;
    DASM_OP(c.sw)
    PROF_G(c_sw)
    #ifdef PROFILERS_EN
    prof.log_stack_access_store(
        (rf[ip.cregh()] + ip.imm_c_mem()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << DASM_CREGL << "," << TO_I32(ip.imm_c_mem())
                << "(" << rf_names[ip.cregh()][rf_names_idx] << ")";
    DASM_MEM_UPDATE_P(TO_I32(ip.imm_c_mem()) + rf[ip.cregh()], ip.cregl());
    #endif
    next_pc = pc + 2;
}

void core::c_swsp() {
    mem->wr(rf[2] + ip.imm_c_swsp(), rf[ip.crs2()], 4u);
    if (tu.is_trapped()) return;
    DASM_OP(c.swsp)
    PROF_G(c_swsp)
    #ifdef PROFILERS_EN
    prof.log_stack_access_store((rf[2] + ip.imm_c_swsp()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << rf_names[ip.crs2()][rf_names_idx] << ","
                << TO_I32(ip.imm_c_swsp())
                << "(" << rf_names[2][rf_names_idx] << ")";
    DASM_MEM_UPDATE_P(TO_I32(ip.imm_c_swsp()) + rf[2], ip.crs2());
    #endif
    next_pc = pc + 2;
}

// C extension - control transfer operations
void core::c_beqz() {
    if (rf[ip.cregh()] == 0) {
        next_pc = pc + ip.imm_c_b();
        PROF_B_T(c_beqz)
    } else {
        next_pc = pc + 2;
        PROF_B_NT(c_beqz, _c_b)
    }
    #ifdef PROFILERS_EN
    branch_taken = (next_pc != (pc + 2));
    #endif
    DASM_OP(c.beqz)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << std::hex << pc + TO_I32(ip.imm_c_b()) << std::dec;
    #endif
}

void core::c_bnez() {
    if (rf[ip.cregh()] != 0) {
        next_pc = pc + ip.imm_c_b();
        PROF_B_T(c_bnez)
    } else {
        next_pc = pc + 2;
        PROF_B_NT(c_beqz, _c_b)
    }
    #ifdef PROFILERS_EN
    branch_taken = (next_pc != (pc + 2));
    #endif
    DASM_OP(c.bnez)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << std::hex << pc + TO_I32(ip.imm_c_b()) << std::dec;
    #endif
}

void core::c_j() {
    next_pc = pc + ip.imm_c_j();
    DASM_OP(c.j)
    PROF_J(c_j)
    #ifdef PROFILERS_EN
    prof_perf.update_jal(next_pc, true, false);
    branch_taken = true;
    #endif
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << std::hex << pc + TO_I32(ip.imm_c_j())
                << std::dec;
    #endif
}

void core::c_jal() {
    next_pc = pc + ip.imm_c_j();
    write_rf(1, pc + 2);
    DASM_OP(c.jal)
    PROF_J(c_jal)
    #ifdef PROFILERS_EN
    prof_perf.update_jal(next_pc, false, false);
    branch_taken = true;
    #endif
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << std::hex << pc + TO_I32(ip.imm_c_j())
                << std::dec;
    DASM_RD_UPDATE_P(1);
    #endif
}

void core::c_jr() {
    // rs1 in position of rd
    if (ip.rd() == 0x0) tu.e_illegal_inst("c.jr (rd=0)", 4);
    next_pc = rf[ip.rd()];
    bool ret_inst = (inst == INST_C_RET);
    if (ret_inst) { DASM_OP(ret) }
    else { DASM_OP(c.jr) }
    PROF_J(c_jr)

    #ifdef PROFILERS_EN
    prof_perf.update_jalr(next_pc, ret_inst, true, pc + 2); // no ra, tail calls
    branch_taken = true;
    #endif

    #ifdef DASM_EN
    if (ret_inst) {
        dasm.asm_ss << dasm.op;
    } else {
        DASM_OP_RD;
    }
    #endif
}

void core::c_jalr() {
    // rs1 in position of rd
    next_pc = rf[ip.rd()];
    uint32_t ra = pc + 2;
    write_rf(1, ra);
    DASM_OP(c.jalr)
    PROF_J(c_jalr)

    #ifdef PROFILERS_EN
    prof_perf.update_jalr(next_pc, false, false, ra);
    branch_taken = true;
    #endif

    #ifdef DASM_EN
    DASM_OP_RD;
    DASM_RD_UPDATE_P(1);
    #endif
}

// C extension - system
void core::c_ebreak() {
    running = false;
    DASM_OP(c.ebreak)
    PROF_G(c_ebreak)
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op;
    #endif
}

// HW stats
#ifdef HW_MODELS_EN
void core::log_hw_stats() {
    std::ofstream ofs;
    ofs.open(cfg.out_dir + "hw_stats.json");
    ofs << "{\n";
    mem->log_cache_stats(ofs);
    bp.log_stats(ofs);
    ofs << "\n\"_done\": true"; // to avoid trailing comma
    ofs << "\n}\n";
    ofs.close();
}
#endif

// Utilities
void core::dump() {
    #ifdef UART_EN
    if (!cfg.sink_uart) std::cout << "=== UART END ===\n";
    #endif
    std::cout << "SIMULATION FINISHED\n\n";

    if (prof_pc.exit_on_prof_stop) {
        std::cout << "Early exit on profiler stop, TOHOST invalid\n";
        csr.at(CSR_TOHOST).value = CSR_TOHOST_EARLY_EXIT;
    } else {
        uint32_t tohost = csr.at(CSR_TOHOST).value;
        if (tohost != 1) {
            std::cout << "Failed test ID: " << (tohost >> 1);
            std::cout << ((tohost > 1000) ? " (trap)" : " (exit)") << "\n";
        }
    }
    std::cout << std::dec << "Instruction Counters: executed: " << inst_cnt
              << ", logged: " << prof_pc.inst_cnt << "\n";
    if (end_dump_state) std::cout << print_state(true) << "\n";
    else std::cout << INDENT << CSRF(csr.find(CSR_TOHOST)) << "\n";

    #ifdef CHECK_LOG
    // open file for check log
    std::ofstream file;
    file.open("sim.check");
    file << std::dec << inst_cnt << "\n";
    for(uint32_t i = 0; i < 32; i++) file << FHEXZ(rf[i], 8) << "\n";
    file << "0x" << MEM_ADDR_FORMAT(pc) << "\n";
    file << FHEXZ(csr.at(CSR_TOHOST).value, 8) << "\n";
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

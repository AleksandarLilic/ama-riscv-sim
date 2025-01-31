#include "core.h"

core::core(
    memory *mem,
    std::string log_path,
    cfg_t cfg,
    [[maybe_unused]] hw_cfg_t hw_cfg) :
        running(false),
        mem(mem), pc(BASE_ADDR), next_pc(0), inst(0),
        inst_cnt(0), inst_cnt_csr(0),
        tu(&csr, &pc, &inst),
        log_path(log_path), logging_pc(cfg.log_pc), logging(false)
    #ifdef ENABLE_PROF
    , prof(log_path)
    , prof_perf(log_path, mem->get_symbol_map(), cfg.perf_event)
    #endif
    #ifdef ENABLE_HW_PROF
    , bp("bpred", hw_cfg)
    , no_bp(hw_cfg.bp_active == bp_t::none)
    #endif
{
    rf[0] = 0;
    rf_names_idx = TO_U8(cfg.rf_names);
    rf_names_w = cfg.rf_names == rf_names_t::mode_abi ? 4 : 3;
    dump_all_regs = cfg.dump_all_regs;
    for (uint32_t i = 1; i < 32; i++) rf[i] = 0xc0ffee;
    mem->trap_setup(&tu);
    #ifdef ENABLE_DASM
    mem->set_dasm(&dasm);
    tu.set_dasm(&dasm);
    #endif
    // initialize CSRs
    csr_names_w = 0;
    for (const auto &c : supported_csrs) {
        csr.insert({c.csr_addr, CSR(c.csr_name, c.boot_val, c.perm)});
        csr_names_w = std::max(csr_names_w, TO_U8(strlen(c.csr_name)));
    }
    #if defined(ENABLE_PROF) && defined(ENABLE_HW_PROF)
    mem->set_perf_profiler(&prof_perf);
    #endif
}

void core::exec() {
    #ifdef ENABLE_DASM
    log_ofstream.open(log_path + "exec.log");
    logging = false;
    #endif

    #ifdef ENABLE_PROF
    prof.active = false;
    prof_perf.active = false;
    prof_fusion.active = false;
    #endif

    #ifdef UART_ENABLE
    std::cout << "=== UART START ===" << "\n";
    #endif

    // start the core
    start_time = std::chrono::high_resolution_clock::now();
    run_time = start_time;
    running = true;
    while (running) exec_inst();

    // wrap up
    cntr_update(); // so that all instructions since last CSR access are counted
    finish(true);
    return;
}

void core::log_and_prof([[maybe_unused]] bool enable) {
    #ifdef ENABLE_DASM
    logging = enable;
    dasm.asm_ss.str(""); // clear the string stream on start/stop
    #endif

    #ifdef ENABLE_PROF
    prof.active = enable;
    prof_perf.active = enable;
    prof_fusion.active = enable;
    #ifdef ENABLE_DASM
    if (enable) log_ofstream << prof_perf.get_callstack_str() << "\n";
    #endif
    #endif

    #ifdef ENABLE_HW_PROF
    logging = enable;
    bp.profiling(enable);
    mem->cache_profiling(enable);
    #endif
}

void core::exec_inst() {
    tu.clear_trap();
    if (logging_pc.should_start(pc)) log_and_prof(true);
    else if (logging_pc.should_stop(pc)) log_and_prof(false);

    inst_fetch();
    uint32_t op_c = ip.copcode();
    if (op_c != 0x3) {
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
    } else {
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

    #ifdef ENABLE_DASM
    #ifdef DPI
    // always log dasm for DPI
    dasm.asm_str = dasm.asm_ss.str();
    dasm.asm_ss.str("");
    if (tu.is_trapped()) return;

    #else // !DPI
    if (logging) {
        dasm.asm_str = dasm.asm_ss.str();
        dasm.asm_ss.str("");
        if (tu.is_trapped()) {
            log_ofstream << dasm.asm_str << "\n";
            return;
        }
        logging_pc.inst_cnt++;
        log_ofstream << INDENT << std::setw(5) << std::setfill(' ')
                     << logging_pc.inst_cnt << ": "
                     << FORMAT_INST(pc, inst, inst_w) << " "
                     << dasm.asm_str << "\n";

        if (logging_pc.dump_state) {
            log_ofstream << dump_state(csr_updated) << "\n";
            csr_updated = false;
        }
    }

    // FIXME: prevent writing in the first place if logging is not active
    dasm.asm_ss.str("");
    if (tu.is_trapped()) return;
    #endif

    #elif defined(ENABLE_HW_PROF)
    if (tu.is_trapped()) return;
    // enable logging in case only HW_PROF is enabled
    if (logging) logging_pc.inst_cnt++;

    #else // !ENABLE_DASM && !ENABLE_HW_PROF
    if (tu.is_trapped()) return;
    #endif

    #ifdef ENABLE_PROF
    if (prof.active) {
        prof.te.pc = pc - BASE_ADDR;
        prof.te.sp = rf[2];
        prof.te.inst_size = TO_U32(inst_w>>1);
        prof.new_inst(inst);
    }
    [[maybe_unused]] bool log_symbol = prof_perf.finish_inst(next_pc);
    #ifdef ENABLE_DASM
    if (log_symbol && logging) {
        log_ofstream << prof_perf.get_callstack_str() << "\n";
    }
    #endif
    #endif

    // next inst
    pc = next_pc;
    inst_cnt++;
}

// void core::reset() {
//     // TODO
// }

void core::finish(bool dump_regs) {
    if (dump_regs) dump();
    #ifdef ENABLE_PROF
    prof_fusion.finish();
    prof_perf.finish();
    prof.finish();
    #endif
    #ifdef ENABLE_HW_PROF
    bp.finish(log_path);
    mem->cache_finish();
    log_hw_stats();
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
    #ifdef ENABLE_DASM
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
    #ifdef ENABLE_DASM
    DASM_OP_RD << "," << DASM_OP_RS1 << ",";
    if (is_shift)
        dasm.asm_ss << FHEXN(ip.imm_i_shamt(), 2);
    else
        dasm.asm_ss << TO_I32(ip.imm_i());
    DASM_RD_UPDATE;
    if (alu_op_sel == TO_U8(alu_i_op_t::op_addi) && (inst == INST_NOP)) {
        dasm.asm_ss = std::ostringstream("nop");
    }
    #endif
}

void core::load() {
    #if defined(ENABLE_PROF) or defined(ENABLE_DASM)
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
    #ifdef ENABLE_PROF
    prof.log_stack_access_load((rs1 + ip.imm_i()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    DASM_OP_RD << "," << TO_I32(ip.imm_i()) << "(" << DASM_OP_RS1 << ")";
    DASM_RD_UPDATE;
    dasm.asm_ss << " <- mem["
                << MEM_ADDR_FORMAT(TO_I32(ip.imm_i()) + rs1) << "]";
    #endif
}

void core::store() {
    switch (ip.funct3()) {
        CASE_STORE(sb)
        CASE_STORE(sh)
        CASE_STORE(sw)
        default: tu.e_unsupported_inst("store");
    }
    #ifdef ENABLE_PROF
    prof.log_stack_access_store((rf[ip.rs1()] + ip.imm_s()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << DASM_OP_RS2 << "," << TO_I32(ip.imm_s())
                << "(" << DASM_OP_RS1 << ")";
    DASM_MEM_UPDATE;
    #endif
}

void core::branch() {
    [[maybe_unused]] bool taken; // unused if built without ENABLE_PROF
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

    #ifdef ENABLE_PROF
    prof_perf.update_branch(next_pc, taken);
    prof_perf.set_perf_event_flag(perf_event_t::branches);
    #endif

    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << DASM_OP_RS1 << "," << DASM_OP_RS2 << ","
                << std::hex << pc + TO_I32(ip.imm_b()) << std::dec;
    #endif

    #ifdef ENABLE_HW_PROF
    last_inst_branch = true;
    bp.ideal(next_pc);
    uint32_t speculative_next_pc = bp.predict(pc, ip.imm_b(), ip.funct3());
    if (!no_bp) { // not accessing icache in case there is no bp, stall instead
        //mem->speculative_exec(speculative_t::enter);
        inst_speculative = mem->rd_inst(speculative_next_pc);
    }
    bp.update(pc, next_pc);
    bool correct = (next_pc == speculative_next_pc);
    if (!correct) {
        //mem->speculative_exec(speculative_t::exit_flush);
        inst_resolved = mem->rd_inst(next_pc);
        #ifdef ENABLE_PROF
        prof_perf.set_perf_event_flag(perf_event_t::bp_mispredict);
        #endif
    } else {
        //mem->speculative_exec(speculative_t::exit_commit);
        inst_resolved = inst_speculative;
    }
    #endif
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
    write_rf(ip.rd(), pc + 4);
    DASM_OP(jalr)
    PROF_J(jalr)
    PROF_RD_RS1

    #ifdef ENABLE_PROF
    bool ret_inst = (inst == INST_RET) || (inst == INST_RET_X5); // but not X15
    prof_perf.update_jalr(next_pc, ret_inst);
    #endif

    #ifdef ENABLE_DASM
    DASM_OP_RD << "," << TO_I32(ip.imm_i()) << "(" << DASM_OP_RS1 << ")";
    DASM_RD_UPDATE;
    #endif
}

void core::jal() {
    write_rf(ip.rd(), pc + 4);
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

    #ifdef ENABLE_PROF
    bool tail_call = (ip.rd() == 0);
    prof_perf.update_jal(next_pc, tail_call);
    #endif

    #ifdef ENABLE_DASM
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
    #ifdef ENABLE_DASM
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
    #ifdef ENABLE_DASM
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
                break;
            default: tu.e_unsupported_inst("system");
        }
        #ifdef ENABLE_DASM
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
    #ifdef ENABLE_DASM
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
        #ifdef ENABLE_PROF
        prof_perf.set_perf_event_flag(perf_event_t::simd);
        #endif
        #ifdef ENABLE_DASM
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
        #ifdef ENABLE_PROF
        prof_perf.set_perf_event_flag(perf_event_t::simd);
        #endif
        #ifdef ENABLE_DASM
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
        #ifdef ENABLE_DASM
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
    int32_t res = 0;
    for (size_t i = 0; i < 2; i++) {
        int32_t sum = TO_I32(TO_I16(a & 0xffff)) + TO_I32(TO_I16(b & 0xffff));
        a >>= 16;
        b >>= 16;
        res |= (sum & 0xFFFF) << (i * 16);
    }
    return res;
}

uint32_t core::al_c_add8(uint32_t a, uint32_t b) {
    // parallel add 4 byte chunks
    int32_t res = 0;
    for (size_t i = 0; i < 4; i++) {
        int32_t sum = TO_I32(TO_I8(a & 0xff)) + TO_I32(TO_I8(b & 0xff));
        a >>= 8;
        b >>= 8;
        res |= (sum & 0xFF) << (i * 8);
    }
    return res;
}

uint32_t core::al_c_sub16(uint32_t a, uint32_t b) {
    // parallel sub 2 halfword chunks
    int32_t res = 0;
    for (size_t i = 0; i < 2; i++) {
        int32_t sum = TO_I32(TO_I16(a & 0xffff)) - TO_I32(TO_I16(b & 0xffff));
        a >>= 16;
        b >>= 16;
        res |= (sum & 0xFFFF) << (i * 16);
    }
    return res;
}

uint32_t core::al_c_sub8(uint32_t a, uint32_t b) {
    // parallel sub 4 byte chunks
    int32_t res = 0;
    for (size_t i = 0; i < 4; i++) {
        int32_t sum = TO_I32(TO_I8(a & 0xff)) - TO_I32(TO_I8(b & 0xff));
        a >>= 8;
        b >>= 8;
        res |= (sum & 0xFF) << (i * 8);
    }
    return res;
}

reg_pair core::al_c_mul16(uint32_t a, uint32_t b) {
    // multiply 2 halfword chunks into 2 32-bit results
    int32_t words[2];
    for (auto &word : words) {
        word = TO_I32(TO_I16(a & 0xffff)) * TO_I32(TO_I16(b & 0xffff));
        a >>= 16;
        b >>= 16;
    }
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::al_c_mul16u(uint32_t a, uint32_t b) {
    // multiply 2 halfword chunks into 2 32-bit results
    uint32_t words[2];
    for (auto &word : words) {
        word = TO_U32(TO_U16(a & 0xffff)) * TO_U32(TO_U16(b & 0xffff));
        a >>= 16;
        b >>= 16;
    }
    return {words[0], words[1]};
}

reg_pair core::al_c_mul8(uint32_t a, uint32_t b) {
    // multiply 4 byte chunks into 2 32-bit results
    int16_t halves[4];
    for (auto &half : halves) {
        half = TO_I16(TO_I8(a & 0xff)) * TO_I16(TO_I8(b & 0xff));
        a >>= 8;
        b >>= 8;
    }
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
    for (auto &half : halves) {
        half = TO_U16(TO_U8(a & 0xff)) * TO_U16(TO_U8(b & 0xff));
        a >>= 8;
        b >>= 8;
    }
    uint32_t words[2] = {0, 0};
    words[0] = (TO_U32(halves[0]) & 0xFFFF) |
               ((TO_U32(halves[1]) & 0xFFFF) << 16);
    words[1] = (TO_U32(halves[2]) & 0xFFFF) |
               ((TO_U32(halves[3]) & 0xFFFF) << 16);
    return {words[0], words[1]};
}

uint32_t core::al_c_dot16(uint32_t a, uint32_t b) {
    // multiply 2 halfword chunks and sum the results
    int32_t res = 0;
    for (size_t i = 0; i < 2; i++) {
        res += TO_I32(TO_I16(a & 0xffff)) * TO_I32(TO_I16(b & 0xffff));
        a >>= 16;
        b >>= 16;
    }
    return res;
}

uint32_t core::al_c_dot8(uint32_t a, uint32_t b) {
    // multiply 4 byte chunks and sum the results
    int32_t res = 0;
    for (size_t i = 0; i < 4; i++) {
        res += TO_I32(TO_I8(a & 0xff)) * TO_I32(TO_I8(b & 0xff));
        a >>= 8;
        b >>= 8;
    }
    return res;
}

uint32_t core::al_c_dot4(uint32_t a, uint32_t b) {
    // multiply 8 nibble chunks and sum the results
    int32_t res = 0;
    for (size_t i = 0; i < 8; i++) {
        res += TO_I32(TO_I4(a & 0xf)) * TO_I32(TO_I4(b & 0xf));
        a >>= 4;
        b >>= 4;
    }
    return res;
}

reg_pair core::mem_c_unpk16(uint32_t a) {
    // unpack 2 16-bit values to 2 32-bit values
    int16_t halves[2];
    for (size_t i = 0; i < 2; i++) {
        halves[i] = TO_I16(a & 0xffff);
        a >>= 16;
    }
    return {TO_U32(halves[0]), TO_U32(halves[1])};
}

reg_pair core::mem_c_unpk16u(uint32_t a) {
    // unpack 2 16-bit values to 2 32-bit values unsigned
    uint16_t halves[2];
    for (size_t i = 0; i < 2; i++) {
        halves[i] = TO_U16(a & 0xffff);
        a >>= 16;
    }
    return {TO_U32(halves[0]), TO_U32(halves[1])};
}

reg_pair core::mem_c_unpk8(uint32_t a) {
    // unpack 4 8-bit values to 4 16-bit values (as 2 32-bit values)
    int8_t bytes[4];
    for (size_t i = 0; i < 4; i++) {
        bytes[i] = TO_I8(a & 0xff);
        a >>= 8;
    }
    int32_t words[2];
    words[0] = (TO_I32(TO_I16(bytes[0])) & 0xFFFF) |
               ((TO_I32(TO_I16(bytes[1])) & 0xFFFF) << 16);
    words[1] = (TO_I32(TO_I16(bytes[2])) & 0xFFFF) |
               ((TO_I32(TO_I16(bytes[3])) & 0xFFFF) << 16);
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::mem_c_unpk8u(uint32_t a) {
    // unpack 4 8-bit values to 4 16-bit values (as 2 32-bit values) unsigned
    uint8_t bytes[4];
    for (size_t i = 0; i < 4; i++) {
        bytes[i] = TO_U8(a & 0xff);
        a >>= 8;
    }
    uint32_t words[2];
    words[0] = (TO_U32(TO_U16(bytes[0])) & 0xFFFF) |
               ((TO_U32(TO_U16(bytes[1])) & 0xFFFF) << 16);
    words[1] = (TO_U32(TO_U16(bytes[2])) & 0xFFFF) |
               ((TO_U32(TO_U16(bytes[3])) & 0xFFFF) << 16);
    return {words[0], words[1]};
}

reg_pair core::mem_c_unpk4(uint32_t a) {
    // unpack 8 4-bit values to 8 8-bit values (as 2 32-bit values)
    int8_t nibbles[8];
    for (size_t i = 0; i < 8; i++) {
        nibbles[i] = TO_I4(a & 0xf);
        a >>= 4;
    }
    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < 4; i++) {
        words[0] |= (TO_I32(TO_I8(nibbles[i])) & 0xFF) << (i * 8);
        words[1] |= (TO_I32(TO_I8(nibbles[i + 4])) & 0xFF) << (i * 8);
    }
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::mem_c_unpk4u(uint32_t a) {
    // unpack 8 4-bit values to 8 8-bit values (as 2 32-bit values) unsigned
    uint8_t nibbles[8];
    for (size_t i = 0; i < 8; i++) {
        nibbles[i] = TO_U4(a & 0xf);
        a >>= 4;
    }
    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < 4; i++) {
        words[0] |= (TO_U32(TO_U8(nibbles[i])) & 0xFF) << (i * 8);
        words[1] |= (TO_U32(TO_U8(nibbles[i + 4])) & 0xFF) << (i * 8);
    }
    return {words[0], words[1]};
}

reg_pair core::mem_c_unpk2(uint32_t a) {
    // unpack 16 2-bit values to 16 4-bit values (as 2 32-bit values)
    int8_t crumbs[16];
    for (size_t i = 0; i < 16; i++) {
        crumbs[i] = TO_I2(a & 0x3);
        a >>= 2;
    }
    int32_t words[2] = {0, 0};
    for (size_t i = 0; i < 8; i++) {
        words[0] |= (TO_I32(TO_I8(crumbs[i])) & 0xF) << (i * 4);
        words[1] |= (TO_I32(TO_I8(crumbs[i + 8])) & 0xF) << (i * 4);
    }
    return {TO_U32(words[0]), TO_U32(words[1])};
}

reg_pair core::mem_c_unpk2u(uint32_t a) {
    // unpack 16 2-bit values to 16 4-bit values (as 2 32-bit values) unsigned
    uint8_t crumbs[16];
    for (size_t i = 0; i < 16; i++) {
        crumbs[i] = TO_U2(a & 0x3);
        a >>= 2;
    }
    uint32_t words[2] = {0, 0};
    for (size_t i = 0; i < 8; i++) {
        words[0] |= (TO_U32(TO_U8(crumbs[i])) & 0xF) << (i * 4);
        words[1] |= (TO_U32(TO_U8(crumbs[i+8])) & 0xF) << (i * 4);
    }
    return {words[0], words[1]};
}

// Zicsr extension
void core::csr_access() {
    cntr_update();
    uint16_t csr_addr = TO_U16(ip.csr_addr());
    auto it = csr.find(csr_addr);
    if (it == csr.end()) {
        // FIXME
        #ifdef ENABLE_DASM
        DASM_TRAP << "Unsupported CSR. Address: " << FHEXN(csr_addr, 3);
        #endif
        SIM_TRAP << "Unsupported CSR. Address: " << FHEXN(csr_addr, 3)
                 << "\n";
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
    #ifdef ENABLE_DASM
    bool imm_type = (ip.funct3() & 0x4);
    DASM_RD_UPDATE;
    if (ip.rs1() || imm_type) {
        if (ip.rd()) dasm.asm_ss << "; ";
        DASM_ALIGN;
        dasm.asm_ss << CSRF(it);
    }
    if (logging_pc.dump_state) csr_updated = true;
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
    run_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>
                   (run_time - start_time);
    mtime = TO_U64(elapsed.count());
    inst_cnt_csr = inst_cnt;

    // user mode shadows
    csr.at(CSR_CYCLE).value = csr.at(CSR_MCYCLE).value;
    csr.at(CSR_CYCLEH).value = csr.at(CSR_MCYCLEH).value;
    csr.at(CSR_INSTRET).value = csr.at(CSR_MINSTRET).value;
    csr.at(CSR_INSTRETH).value = csr.at(CSR_MINSTRETH).value;
    csr.at(CSR_TIME).value = TO_U32(mtime);
    csr.at(CSR_TIMEH).value = TO_U32(mtime >> 32);
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
    #ifdef ENABLE_DASM
    DASM_OP_RD << "," << TO_I32(ip.imm_c_arith());
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_li() {
    write_rf(ip.rd(), ip.imm_c_arith());
    DASM_OP(c.li)
    PROF_G(c_li)
    #ifdef ENABLE_DASM
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
    #ifdef ENABLE_DASM
    DASM_OP_RD << "," << FHEXN((ip.imm_c_lui() >> 12), 5);
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_nop() {
    // write_rf(0, al_addi(rf[0], 0));
    DASM_OP(c.nop)
    PROF_G(c_nop)
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op;
    #endif
    next_pc = pc + 2;
}

void core::c_addi16sp() {
    if (ip.imm_c_16sp() == 0) tu.e_illegal_inst("c.addi16sp (imm=0)", 4);
    write_rf(2, al_addi(rf[2], ip.imm_c_16sp()));
    DASM_OP(c.addi16sp)
    PROF_G(c_addi16sp)
    #ifdef ENABLE_DASM
    DASM_OP_RD << "," << TO_I32(ip.imm_c_16sp());
    DASM_RD_UPDATE_P(2);
    #endif
    next_pc = pc + 2;
}

void core::c_srli() {
    write_rf(ip.cregh(), al_srli(rf[ip.cregh()], ip.imm_c_arith()));
    DASM_OP(c.srli)
    PROF_G(c_srli)
    #ifdef ENABLE_DASM
    DASM_OP_CREGH << "," << TO_I32(ip.imm_c_arith());
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_srai() {
    write_rf(ip.cregh(), al_srai(rf[ip.cregh()], ip.imm_c_arith()));
    DASM_OP(c.srai)
    PROF_G(c_srai)
    #ifdef ENABLE_DASM
    DASM_OP_CREGH << "," << TO_I32(ip.imm_c_arith());
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_andi() {
    write_rf(ip.cregh(), al_andi(rf[ip.cregh()], ip.imm_c_arith()));
    DASM_OP(c.andi)
    PROF_G(c_andi)
    #ifdef ENABLE_DASM
    DASM_OP_CREGH << "," << TO_I32(ip.imm_c_arith());
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_and() {
    write_rf(ip.cregh(), al_and(rf[ip.cregh()], rf[ip.cregl()]));
    DASM_OP(c.and)
    PROF_G(c_and)
    #ifdef ENABLE_DASM
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_or() {
    write_rf(ip.cregh(), al_or(rf[ip.cregh()], rf[ip.cregl()]));
    DASM_OP(c.or)
    PROF_G(c_or)
    #ifdef ENABLE_DASM
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_xor() {
    write_rf(ip.cregh(), al_xor(rf[ip.cregh()], rf[ip.cregl()]));
    DASM_OP(c.xor)
    PROF_G(c_xor)
    #ifdef ENABLE_DASM
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.cregh());
    #endif
    next_pc = pc + 2;
}

void core::c_sub() {
    write_rf(ip.cregh(), al_sub(rf[ip.cregh()], rf[ip.cregl()]));
    DASM_OP(c.sub)
    PROF_G(c_sub)
    #ifdef ENABLE_DASM
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
    #ifdef ENABLE_DASM
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
    #ifdef ENABLE_PROF
    prof_fusion.attack({trigger::slli_lea, inst, mem->rd_inst(pc + 2), true});
    #endif
    #ifdef ENABLE_DASM
    DASM_OP_RD << "," << FHEXN(TO_I32(ip.imm_c_slli()), 2);
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_mv() {
    write_rf(ip.rd(), rf[ip.crs2()]);
    DASM_OP(c.mv)
    PROF_G(c_mv)
    #ifdef ENABLE_DASM
    DASM_OP_RD << "," << rf_names[ip.crs2()][rf_names_idx];
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_add() {
    write_rf(ip.rd(), al_add(rf[ip.rd()], rf[ip.crs2()]));
    DASM_OP(c.add)
    PROF_G(c_add)
    #ifdef ENABLE_DASM
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
    #ifdef ENABLE_PROF
    prof.log_stack_access_load((rs1 + ip.imm_c_mem()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << DASM_CREGL << "," << TO_I32(ip.imm_c_mem())
                << "(" << rf_names[ip.cregh()][rf_names_idx] << ")";
    DASM_RD_UPDATE_P(ip.cregl());
    dasm.asm_ss << " <- mem["
                << MEM_ADDR_FORMAT(TO_I32(ip.imm_c_mem()) + rs1) << "]";
    #endif
    next_pc = pc + 2;
}

void core::c_lwsp() {
    uint32_t loaded = mem->rd(rf[2] + ip.imm_c_lwsp(), 4u);
    if (tu.is_trapped()) return;
    write_rf(ip.rd(), loaded);
    DASM_OP(c.lwsp)
    PROF_G(c_lwsp)
    #ifdef ENABLE_PROF
    prof.log_stack_access_load((rf[2] + ip.imm_c_lwsp()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef ENABLE_DASM
    DASM_OP_RD << "," << TO_I32(ip.imm_c_lwsp())
               << "(" << rf_names[2][rf_names_idx] << ")";
    DASM_RD_UPDATE;
    dasm.asm_ss << " <- mem[" << MEM_ADDR_FORMAT(TO_I32(ip.imm_c_lwsp())+rf[2])
                << "]";
    #endif
    next_pc = pc + 2;
}

void core::c_sw() {
    mem->wr(rf[ip.cregh()] + ip.imm_c_mem(), rf[ip.cregl()], 4u);
    if (tu.is_trapped()) return;
    DASM_OP(c.sw)
    PROF_G(c_sw)
    #ifdef ENABLE_PROF
    prof.log_stack_access_store(
        (rf[ip.cregh()] + ip.imm_c_mem()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef ENABLE_DASM
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
    #ifdef ENABLE_PROF
    prof.log_stack_access_store((rf[2] + ip.imm_c_swsp()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef ENABLE_DASM
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
    DASM_OP(c.beqz)
    #ifdef ENABLE_DASM
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
    DASM_OP(c.bnez)
    #ifdef ENABLE_DASM
    DASM_OP_CREGH << "," << std::hex << pc + TO_I32(ip.imm_c_b()) << std::dec;
    #endif
}

void core::c_j() {
    next_pc = pc + ip.imm_c_j();
    DASM_OP(c.j)
    PROF_J(c_j)
    #ifdef ENABLE_PROF
    prof_perf.update_jal(next_pc, true);
    #endif
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << std::hex << pc + TO_I32(ip.imm_c_j())
                << std::dec;
    #endif
}

void core::c_jal() {
    next_pc = pc + ip.imm_c_j();
    write_rf(1, pc + 2);
    DASM_OP(c.jal)
    PROF_J(c_jal)
    #ifdef ENABLE_PROF
    prof_perf.update_jal(next_pc, false);
    #endif
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << std::hex << pc + TO_I32(ip.imm_c_j())
                << std::dec;
    DASM_RD_UPDATE_P(1);
    #endif
}

void core::c_jr() {
    // rs1 in position of rd
    if (ip.rd() == 0x0) tu.e_illegal_inst("c.jr (rd=0)", 4);
    next_pc = rf[ip.rd()];
    DASM_OP(c.jr)
    PROF_J(c_jr)
    #ifdef ENABLE_PROF
    bool ret_inst = (inst == INST_C_RET);
    prof_perf.update_jalr(next_pc, ret_inst);
    #endif
    #ifdef ENABLE_DASM
    DASM_OP_RD;
    #endif
}

void core::c_jalr() {
    // rs1 in position of rd
    next_pc = rf[ip.rd()];
    write_rf(1, pc + 2);
    DASM_OP(c.jalr)
    PROF_J(c_jalr)
    #ifdef ENABLE_PROF
    prof_perf.update_jalr(next_pc, false);
    #endif
    #ifdef ENABLE_DASM
    DASM_OP_RD;
    DASM_RD_UPDATE_P(1);
    #endif
}

// C extension - system
void core::c_ebreak() {
    running = false;
    DASM_OP(c.ebreak)
    PROF_G(c_ebreak)
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op;
    #endif
}

// HW stats
#ifdef ENABLE_HW_PROF
void core::log_hw_stats() {
    std::ofstream ofs;
    ofs.open(log_path + "hw_stats.json");
    ofs << "{\n";
    mem->log_cache_stats(ofs);
    bp.log_stats(ofs);
    ofs << "\"_done\": true"; // to avoid trailing comma
    ofs << "\n}\n";
    ofs.close();
}
#endif

// Utilities
void core::dump() {
    #ifdef UART_ENABLE
    std::cout << "=== UART END ===\n" << "\n";
    #endif
    uint32_t tohost = csr.at(CSR_TOHOST).value;
    if (tohost != 1) {
        std::cout << "Failed test ID: " << (tohost >> 1);
        std::cout << ((tohost > 1000) ? " (trap)" : " (exit)") << "\n";
    }
    std::cout << std::dec << "Instruction Counters: executed: " << inst_cnt
              << ", logged: " << logging_pc.inst_cnt << "\n";
    if (dump_all_regs) std::cout << dump_state(true) << "\n";
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

std::string core::dump_state(bool dump_csr) {
    std::ostringstream state;
    state << INDENT << "PC: " << MEM_ADDR_FORMAT(pc) << "\n";
    for(uint32_t i = 0; i < 32; i+=4){
        state << INDENT;
        for(uint32_t j = 0; j < 4; j++) {
            state << FRF(rf_names[i+j][rf_names_idx], rf[i+j]) << "   ";
        }
        state << "\n";
    }
    if (dump_csr) {
        for (auto it = csr.begin(); it != csr.end(); it++) {
            state << INDENT << CSRF(it) << "\n";
        }
    }

    return state.str();
}

#include "core.h"

#define INDENT "    "

core::core(uint32_t base_address, memory *mem, std::string log_name)
    #ifdef ENABLE_PROF
    : prof(log_name)
    #endif
{
    inst_cnt = 0;
    pc = base_address;
    next_pc = 0;
    this->mem = mem;
    this->log_name = log_name;
    for (uint32_t i = 0; i < 32; i++) rf[i] = 0;
    // initialize CSRs
    for (const auto &c : supported_csrs)
        csr.insert({c.csr_addr, CSR(c.csr_name, 0x0)});
}

void core::exec() {
    #if defined(LOG_EXEC) or defined(LOG_EXEC_ALL)
    log_ofstream.open(log_name + "_exec.log");
    #endif
    running = true;
    while (running) exec_inst();
    finish(true);
    return;
}

void core::exec_inst() {
    inst = mem->get_inst(pc);
    #ifdef ENABLE_PROF
    prof.new_inst(inst);
    prof.log((pc - BASE_ADDR)>>2, rf[2]);
    #endif
    switch (get_opcode()) {
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
        default: unsupported("opcode");
    }

    #ifdef ENABLE_DASM
    dasm.asm_str = dasm.asm_ss.str();
    dasm.asm_ss.str("");

    #if defined(LOG_EXEC) or defined(LOG_EXEC_ALL)
    log_ofstream << FORMAT_INST(inst) << " " << dasm.asm_str << std::endl;
    #endif

    #ifdef LOG_EXEC_ALL
    log_ofstream << mem_ostr.str();
    mem_ostr.str("");
    log_ofstream << dump_state() << std::endl;
    #endif

    #endif
    pc = next_pc;
    inst_cnt++;
}

// void core::reset() {
//     // TODO
// }

void core::finish(bool dump_regs) {
    if (dump_regs) dump();
}

/*
 * Integer extension
 */
void core::al_reg() {
    bool is_mul = get_funct7_b1() == 1;
    uint32_t funct3 = get_funct3();
    uint32_t alu_op_sel = ((get_funct7_b5()) << 3) | funct3;
    if (!is_mul) {
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
            default: unsupported("al_reg");
        }
    } else {
        switch (funct3) {
            CASE_ALU_REG_MUL_OP(mul)
            CASE_ALU_REG_MUL_OP(mulh)
            CASE_ALU_REG_MUL_OP(mulhsu)
            CASE_ALU_REG_MUL_OP(mulhu)
            CASE_ALU_REG_MUL_OP(div)
            CASE_ALU_REG_MUL_OP(divu)
            CASE_ALU_REG_MUL_OP(rem)
            CASE_ALU_REG_MUL_OP(remu)
            default: unsupported("al_reg_mul");
        }
    }
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << rf_names[get_rs1()][RF_NAMES] << ","
                << rf_names[get_rs2()][RF_NAMES];
    #endif
}

void core::al_imm() {
    uint32_t alu_op_sel_shift = ((get_funct7_b5()) << 3) | get_funct3();
    bool is_shift = (get_funct3() & 0x3) == 1;
    uint32_t alu_op_sel = is_shift ? alu_op_sel_shift : get_funct3();
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
        default: unsupported("al_imm");
    }
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << rf_names[get_rs1()][RF_NAMES] << ",";
    if (is_shift)
        dasm.asm_ss << std::hex << "0x" << get_imm_i_shamt() << std::dec;
    else
        dasm.asm_ss << (int)get_imm_i();
    #endif
}

void core::load() {
    switch (get_funct3()) {
        CASE_LOAD(lb)
        CASE_LOAD(lh)
        CASE_LOAD(lw)
        CASE_LOAD(lbu)
        CASE_LOAD(lhu)
        default: unsupported("load");
    }
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << (int)get_imm_i()
                << "(" << rf_names[get_rs1()][RF_NAMES] << ")";
    #endif
    #ifdef LOG_EXEC_ALL
    mem_ostr << INDENT << "mem["
             << MEM_ADDR_FORMAT((int)get_imm_i() + rf[get_rs1()]) << "] (0x"
             << rf[get_rd()]<< ") -> "
             << rf_names[get_rd()][RF_NAMES] << std::endl;
    #endif
}

void core::store() {
    switch (get_funct3()) {
        CASE_STORE(sb)
        CASE_STORE(sh)
        CASE_STORE(sw)
        default: unsupported("store");
    }
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rs2()][RF_NAMES] << ","
                << (int)get_imm_s()
                << "(" << rf_names[get_rs1()][RF_NAMES] << ")";
    #endif
    #ifdef LOG_EXEC_ALL
    mem_ostr << INDENT << rf_names[get_rs2()][RF_NAMES]
             << " (0x" << rf[get_rs2()] << ") -> mem["
             << MEM_ADDR_FORMAT((int)get_imm_s() + rf[get_rs1()]) << "]"
             << std::endl;
    #endif
}

void core::branch() {
    next_pc = pc + 4;
    switch (get_funct3()) {
        CASE_BRANCH(beq)
        CASE_BRANCH(bne)
        CASE_BRANCH(blt)
        CASE_BRANCH(bge)
        CASE_BRANCH(bltu)
        CASE_BRANCH(bgeu)
        default: unsupported("branch");
    }
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rs1()][RF_NAMES] << ","
                << rf_names[get_rs2()][RF_NAMES] << ","
                << std::hex << pc + (int)get_imm_b() << std::dec;
    #endif
}

void core::jalr() {
    if (get_funct3() != 0) unsupported("jalr");
    next_pc = (rf[get_rs1()] + get_imm_i()) & 0xFFFFFFFE;
    DASM_OP(jalr)
    PROF_J(jalr)
    write_rf(get_rd(), pc + 4);
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << (int)get_imm_i()
                << "(" << rf_names[get_rs1()][RF_NAMES] << ")";
    #endif
}

void core::jal() {
    write_rf(get_rd(), pc + 4);
    next_pc = pc + get_imm_j();
    DASM_OP(jal)
    PROF_J(jal)
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << std::hex << pc + (int)get_imm_j() << std::dec;
    #endif
}

void core::lui() {
    write_rf(get_rd(), get_imm_u());
    next_pc = pc + 4;
    DASM_OP(lui)
    PROF_UPP(lui);
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << "0x" << std::hex << (get_imm_u() >> 12) << std::dec;
    #endif
}

void core::auipc() {
    write_rf(get_rd(), get_imm_u() + pc);
    next_pc = pc + 4;
    DASM_OP(auipc)
    PROF_UPP(auipc);
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << "0x" << std::hex << (get_imm_u() >> 12) << std::dec;
    #endif
}

void core::system() {
    if (inst == INST_ECALL) {
        running = false;
        DASM_OP(ecall)
        PROF_SYS(ecall)
        #ifdef ENABLE_DASM
        dasm.asm_ss << dasm.op;
        #endif
    }
    else if (inst == INST_EBREAK) {
        running = false;
        DASM_OP(ebreak)
        PROF_SYS(ebreak)
        #ifdef ENABLE_DASM
        dasm.asm_ss << dasm.op;
        #endif
    } else {
        csr_access();
        next_pc = pc + 4;
    }
}

void core::misc_mem() {
    if (inst == INST_FENCE_I) {
        // nop
        next_pc = pc + 4;
        DASM_OP(fence.i)
        PROF_MEM(fence_i)
        #ifdef ENABLE_DASM
        dasm.asm_ss << dasm.op;
        #endif
    } else if (get_funct3() == 0) {
        // nop
        next_pc = pc + 4;
        DASM_OP(fence)
        PROF_MEM(fence)
        #ifdef ENABLE_DASM
        dasm.asm_ss << dasm.op;
        #endif
    } else {
        unsupported("misc_mem");
    }
}

/*
 * Zicsr extension
 */
void core::csr_access() {
    uint32_t csr_address = get_csr_addr();
    auto it = csr.find(csr_address);
    if (it == csr.end()) {
        std::cerr << "Unsupported CSR. Address: 0x" << std::hex << csr_address
                  << std::dec <<std::endl;
        throw std::runtime_error("Unsupported CSR");
    } else {
        // using temp in case rd and rs1 are the same register
        uint32_t init_val_rs1 = rf[get_rs1()];
        write_rf(get_rd(), it->second.value);
        switch (get_funct3()) {
            CASE_CSR(rw, init_val_rs1)
            CASE_CSR(rs, init_val_rs1)
            CASE_CSR(rc, init_val_rs1)
            CASE_CSR_I(rwi)
            CASE_CSR_I(rsi)
            CASE_CSR_I(rci)
            default: unsupported("sys");
        }
        #ifdef ENABLE_DASM
        CSR_REG_DASM;
        #endif
        // TODO: change to 'tohost' CSR
        if (csr.at(0x340).value & 0x1) running = false;
    }
}

void core::unsupported(const std::string &msg) {
    std::cerr << "Unsupported instruction: <" << msg << "> "
              << FORMAT_INST(inst) << std::endl;
    throw std::runtime_error("Unsupported instruction");
}

/*
 * Utilities
 */
void core::dump() {
    #ifdef UART_ENABLE
    std::cout << std::endl;
    #endif
    std::cout << std::dec << "Inst Counter: " << inst_cnt << std::endl;
    std::cout << dump_state() << std::endl;

    #ifdef CHECK_LOG
    // open file for check log
    std::ofstream file;
    file.open("sim.check");
    file << std::dec << inst_cnt << std::endl;
    for(uint32_t i = 0; i < 32; i++){
        file << "0x" << std::setw(8) << std::setfill('0')
             << std::hex << rf[i] << std::endl;
    }
    file << "0x" << MEM_ADDR_FORMAT(pc) << std::endl;
    file << "0x" << std::setw(8) << std::setfill('0')
         << std::hex << csr.at(0x340).value << std::endl;
    file.close();
    #endif
}

std::string core::dump_state() {
    std::ostringstream state;
    state << INDENT << "PC: " << MEM_ADDR_FORMAT(pc) << std::endl;
    for(uint32_t i = 0; i < 32; i+=4){
        state << INDENT;
        for(uint32_t j = 0; j < 4; j++) {
            state << FRF(rf_names[i+j][RF_NAMES], rf[i+j]);
        }
        state << std::endl;
    }
    for (auto it = csr.begin(); it != csr.end(); it++)
        state << INDENT << CSRF(it) << std::endl;

    return state.str();
}

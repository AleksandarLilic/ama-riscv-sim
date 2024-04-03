#include "core.h"

core::core(uint32_t base_address, memory *mem, std::string log_name)
    #ifdef ENABLE_PROF
    : prof(log_name)
    #endif
{
    inst_cnt = 0;
    pc = base_address;
    next_pc = 0;
    this->mem = mem;
    for (uint32_t i = 0; i < 32; i++) rf[i] = 0;
    // initialize CSRs
    for (const auto &c : supported_csrs)
        csr.insert({c.csr_addr, CSR(c.csr_name, 0x0)});
}

void core::exec() {
    running = true;
    while (running) exec_inst();
    dump();
    #ifdef ENABLE_PROF
    prof.log_to_file();
    #endif
    return;
}

void core::exec_inst() {
    inst = mem->get_inst(pc);
    #ifdef ENABLE_PROF
    prof.new_inst(inst);
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
        default: unsupported();
    }
    #ifdef ENABLE_DASM
    dasm.asm_str = dasm.asm_ss.str();
    dasm.asm_ss.str("");
    #ifdef PRINT_EXEC
    PRINT_INST(inst) << " " << dasm.asm_str << std::endl;
    #endif
    #endif
    pc = next_pc;
    inst_cnt++;
}

void core::reset() {
    // TODO
}

/*
 * Integer extension
 */
void core::al_reg() {
    uint32_t alu_op_sel = ((get_funct7_b5()) << 3) | get_funct3();
    PROF_AL_TYPE(reg)
    switch (alu_op_sel) {
        CASE_ALU_OP(add)
        CASE_ALU_OP(sub)
        CASE_ALU_OP(sll)
        CASE_ALU_OP(srl)
        CASE_ALU_OP(sra)
        CASE_ALU_OP(slt)
        CASE_ALU_OP(sltu)
        CASE_ALU_OP(xor)
        CASE_ALU_OP(or)
        CASE_ALU_OP(and)
        default: unsupported();
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
    PROF_AL_TYPE(imm)
    switch (alu_op_sel) {
        CASE_ALU_OP_IMM(add)
        CASE_ALU_OP_IMM(sll)
        CASE_ALU_OP_IMM(srl)
        CASE_ALU_OP_IMM(sra)
        CASE_ALU_OP_IMM(slt)
        CASE_ALU_OP_IMM(sltu)
        CASE_ALU_OP_IMM(xor)
        CASE_ALU_OP_IMM(or)
        CASE_ALU_OP_IMM(and)
        default: unsupported();
    }
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    if (dasm.op == "sltu")
        dasm.op = "sltiu ";
    else
        dasm.op += "i ";
    dasm.asm_ss << dasm.op << rf_names[get_rd()][RF_NAMES] << ","
                << rf_names[get_rs1()][RF_NAMES] << ",";
    if (is_shift)
        dasm.asm_ss << std::hex << "0x" << get_imm_i_shamt() << std::dec;
    else
        dasm.asm_ss << (int)get_imm_i();
    #endif
}

void core::load() {
    switch (get_funct3()) {
        CASE_LOAD(byte)
        CASE_LOAD(half)
        CASE_LOAD(word)
        CASE_LOAD(byte_u)
        CASE_LOAD(half_u)
        default: unsupported();
    }
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << (int)get_imm_i() 
                << "(" << rf_names[get_rs1()][RF_NAMES] << ")";
    #endif
}

void core::store() {
    switch (get_funct3()) {
        CASE_STORE(byte)
        CASE_STORE(half)
        CASE_STORE(word)
    }
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rs2()][RF_NAMES] << ","
                << (int)get_imm_s() 
                << "(" << rf_names[get_rs1()][RF_NAMES] << ")";
    #endif
}

void core::branch() {
    next_pc = pc + 4;
    switch (get_funct3()) {
        CASE_BRANCH(eq)
        CASE_BRANCH(ne)
        CASE_BRANCH(lt)
        CASE_BRANCH(ge)
        CASE_BRANCH(ltu)
        CASE_BRANCH(geu)
        default: unsupported();
    }
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rs1()][RF_NAMES] << ","
                << rf_names[get_rs2()][RF_NAMES] << ","
                << std::hex << pc + (int)get_imm_b() << std::dec;
    #endif
}

void core::jalr() {
    DASM_OP("jalr")
    next_pc = (rf[get_rs1()] + get_imm_i()) & 0xFFFFFFFE;
    PROF_J(jalr)
    write_rf(get_rd(), pc + 4);
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << (int)get_imm_i()
                << "(" << rf_names[get_rs1()][RF_NAMES] << ")";
    #endif
}

void core::jal() {
    DASM_OP("jal")
    write_rf(get_rd(), pc + 4);
    next_pc = pc + get_imm_j();
    PROF_J(jal)
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << std::hex << pc + (int)get_imm_j() << std::dec;
    #endif
}

void core::lui() {
    DASM_OP("lui")
    PROF_UPP(lui);
    write_rf(get_rd(), get_imm_u());
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << "0x" << std::hex << (get_imm_u() >> 12) << std::dec;
    #endif
}

void core::auipc() {
    DASM_OP("auipc")
    PROF_UPP(auipc);
    write_rf(get_rd(), get_imm_u() + pc);
    next_pc = pc + 4;
    #ifdef ENABLE_DASM
    dasm.asm_ss << dasm.op << " " << rf_names[get_rd()][RF_NAMES] << ","
                << "0x" << std::hex << (get_imm_u() >> 12) << std::dec;
    #endif
}

void core::system() {
    if (inst == INST_ECALL) {
        running = false;
        PROF_SYS(ecall)
        #ifdef ENABLE_DASM
        DASM_OP("ecall")
        #endif
    }
    else if (inst == INST_EBREAK) {
        running = false;
        PROF_SYS(ebreak)
        #ifdef ENABLE_DASM
        DASM_OP("ebreak")
        dasm.asm_ss << dasm.op;
        #endif
    } else {
        csr_access();
        next_pc = pc + 4;
    }
}

void core::misc_mem() {
    if (inst == INST_FENCE) {
        // nop
        next_pc = pc + 4;
        #ifdef ENABLE_DASM
        DASM_OP("fence.i")
        dasm.asm_ss << dasm.op;
        #endif
    } else {
        unsupported();
    }
}

void core::unsupported() {
    std::cerr << "Unsupported instruction: ";
    PRINT_INST(inst);
    std::cout << std::endl;
    throw std::runtime_error("Unsupported instruction");
}

/*
 * Zicsr extension
 */
void core::csr_access() {
    uint32_t csr_address = get_csr_addr();
    auto it = csr.find(csr_address);
    if (it == csr.end()) {
        std::cerr << "Unsupported CSR. Address: " << std::hex << csr_address 
                  << std::dec <<std::endl;
        throw std::runtime_error("Unsupported CSR");
    } else {
        // using temp in case rd and rs1 are the same register
        uint32_t init_val_rs1 = rf[get_rs1()];
        write_rf(get_rd(), it->second.value);
        switch (get_funct3()) {
            case (uint8_t)csr_op_t::op_rw: csr_rw(init_val_rs1); break;
            case (uint8_t)csr_op_t::op_rs: csr_rs(init_val_rs1); break;
            case (uint8_t)csr_op_t::op_rc: csr_rc(init_val_rs1); break;
            case (uint8_t)csr_op_t::op_rwi: csr_rwi(); break;
            case (uint8_t)csr_op_t::op_rsi: csr_rsi(); break;
            case (uint8_t)csr_op_t::op_rci: csr_rci(); break;
            default: unsupported();
        }
        #ifdef ENABLE_DASM
        CSR_REG_DASM;
        #endif
    }
}

/*
 * Utilities
 */
void core::dump() {
    std::cout << std::dec << "Inst Counter: " << inst_cnt << std::endl;
    std::cout << "PC: " << MEM_ADDR_FORMAT(pc) << std::endl;
    for(uint32_t i = 0; i < 32; i+=4){
        for(uint32_t j = 0; j < 4; j++) {
            std::cout << FRF(rf_names[i+j][RF_NAMES], rf[i+j]);
        }
        std::cout << std::endl;
    }
    for (auto it = csr.begin(); it != csr.end(); it++)
        std::cout << CSRF(it) << std::endl;
    
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

/*
 * Instruction parsing
 */
uint32_t core::get_opcode() { return (inst & M_OPC7); }
uint32_t core::get_funct7() { return (inst & M_FUNCT7) >> 25; }
uint32_t core::get_funct7_b5() { return (inst & M_FUNCT7_B5) >> 30; }
uint32_t core::get_funct3() { return (inst & M_FUNCT3) >> 12; }
uint32_t core::get_rd() { return (inst & M_RD) >> 7; }
uint32_t core::get_rs1() { return (inst & M_RS1) >> 15; }
uint32_t core::get_rs2() { return (inst & M_RS2) >> 20; }
uint32_t core::get_imm_i() { return int32_t(inst & M_IMM_31_20) >> 20; }
uint32_t core::get_imm_i_shamt() { return (inst & M_IMM_24_20) >> 20; }
uint32_t core::get_csr_addr() { return (inst & M_IMM_31_20) >> 20; }
uint32_t core::get_uimm_csr() { return get_rs1(); }
uint32_t core::get_imm_s() { 
    return ((int32_t(inst) & M_IMM_31_25) >> 20) |
        ((inst & M_IMM_11_8) >> 7) |
        ((inst & M_IMM_7) >> 7);
}

uint32_t core::get_imm_b() { 
    return ((int32_t(inst) & M_IMM_31) >> 19) |
        ((inst & M_IMM_7) << 4) |
        ((inst & M_IMM_30_25) >> 20) |
        ((inst & M_IMM_11_8) >> 7);
}

uint32_t core::get_imm_j() {
    return ((int32_t(inst) & M_IMM_31) >> 11) |
        ((inst & M_IMM_19_12)) |
        ((inst & M_IMM_20) >> 9) |
        ((inst & M_IMM_30_25) >> 20) |
        ((inst & M_IMM_24_21) >> 20);
}

uint32_t core::get_imm_u() {
    return ((int32_t(inst) & M_IMM_31_20)) |
        ((inst & M_IMM_19_12));
}

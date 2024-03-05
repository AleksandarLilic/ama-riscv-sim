#include "core.h"

core::core(uint32_t base_address, memory *mem) {
    inst_cnt = 0;
    pc = base_address;
    next_pc = 0;
    this->mem = mem;
    for (uint32_t i = 0; i < 32; i++) rf[i] = 0;
    // initialize CSRs
    csr[CSR_TOHOST] = 0x0;
    csr[CSR_MSCRATCH] = 0x0;
    // instruction decoders
    decoder_op_map[(uint8_t)opcode::al_reg] = &core::al_reg;
    decoder_op_map[(uint8_t)opcode::al_imm] = &core::al_imm;
    decoder_op_map[(uint8_t)opcode::load] = &core::load;
    decoder_op_map[(uint8_t)opcode::store] = &core::store;
    decoder_op_map[(uint8_t)opcode::branch] = &core::branch;
    decoder_op_map[(uint8_t)opcode::jalr] = &core::jalr;
    decoder_op_map[(uint8_t)opcode::jal] = &core::jal;
    decoder_op_map[(uint8_t)opcode::lui] = &core::lui;
    decoder_op_map[(uint8_t)opcode::auipc] = &core::auipc;
    decoder_op_map[(uint8_t)opcode::system] = &core::system;
    // alu operations
    alu_op_map[(uint8_t)alu_op_t::op_add] = &core::al_add;
    alu_op_map[(uint8_t)alu_op_t::op_sub] = &core::al_sub;
    alu_op_map[(uint8_t)alu_op_t::op_sll] = &core::al_sll;
    alu_op_map[(uint8_t)alu_op_t::op_srl] = &core::al_srl;
    alu_op_map[(uint8_t)alu_op_t::op_sra] = &core::al_sra;
    alu_op_map[(uint8_t)alu_op_t::op_slt] = &core::al_slt;
    alu_op_map[(uint8_t)alu_op_t::op_sltu] = &core::al_sltu;
    alu_op_map[(uint8_t)alu_op_t::op_xor] = &core::al_xor;
    alu_op_map[(uint8_t)alu_op_t::op_or] = &core::al_or;
    alu_op_map[(uint8_t)alu_op_t::op_and] = &core::al_and;
    // load operations
    load_op_map[(uint8_t)load_op_t::op_byte] = &core::load_byte;
    load_op_map[(uint8_t)load_op_t::op_half] = &core::load_half;
    load_op_map[(uint8_t)load_op_t::op_word] = &core::load_word;
    load_op_map[(uint8_t)load_op_t::op_byte_u] = &core::load_byte_u;
    load_op_map[(uint8_t)load_op_t::op_half_u] = &core::load_half_u;
    // store operations
    store_op_map[(uint8_t)store_op_t::op_byte] = &core::store_byte;
    store_op_map[(uint8_t)store_op_t::op_half] = &core::store_half;
    store_op_map[(uint8_t)store_op_t::op_word] = &core::store_word;
    // branch operations
    branch_op_map[(uint8_t)branch_op_t::op_beq] = &core::branch_eq;
    branch_op_map[(uint8_t)branch_op_t::op_bne] = &core::branch_ne;
    branch_op_map[(uint8_t)branch_op_t::op_blt] = &core::branch_lt;
    branch_op_map[(uint8_t)branch_op_t::op_bge] = &core::branch_ge;
    branch_op_map[(uint8_t)branch_op_t::op_bltu] = &core::branch_ltu;
    branch_op_map[(uint8_t)branch_op_t::op_bgeu] = &core::branch_geu;
    // csr operations
    csr_op_map[(uint8_t)csr_op_t::op_csrrw] = &core::csr_read_write;
    csr_op_map[(uint8_t)csr_op_t::op_csrrwi] = &core::csr_read_write_imm;
    csr_op_map[(uint8_t)csr_op_t::op_csrrs] = &core::csr_read_set;
    csr_op_map[(uint8_t)csr_op_t::op_csrrc] = &core::csr_read_clear;
    csr_op_map[(uint8_t)csr_op_t::op_csrrsi] = &core::csr_read_set_imm;
    csr_op_map[(uint8_t)csr_op_t::op_csrrci] = &core::csr_read_clear_imm;
}

void core::exec() {
    running = true;
    inst = mem->rd32(pc);
    while (running){
#ifdef PRINT_EXEC
    PRINT_INST(inst);
#endif
        auto inst_dec = decoder_op_map.find(get_opcode());
        if (inst_dec != decoder_op_map.end()) (this->*inst_dec->second)();
        else unsupported();
        pc = next_pc;
#ifdef PRINT_EXEC
        std::cout << std::endl;
#endif
        inst_cnt++;
        inst = mem->rd32(pc);
    }
    PRINT_INST(inst);
    std::cout << std::endl;
    dump();
    return;
}

void core::exec_inst() {
    inst = mem->rd32(pc);
    PRINT_INST(inst) << std::endl;
    auto inst_dec = decoder_op_map.find(get_opcode());
    if (inst_dec != decoder_op_map.end()) (this->*inst_dec->second)();
    else unsupported();
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
#ifdef PRINT_EXEC
    std::cout << "  Arith Logic REG ";
#endif
    uint32_t alu_op_sel = ((get_funct7_b5()) << 3) | get_funct3();
    write_rf(get_rd(), 
        (this->*alu_op_map[alu_op_sel])(rf[get_rs1()], rf[get_rs2()]));
    next_pc = pc + 4;
}

void core::al_imm() {
#ifdef PRINT_EXEC
    std::cout << "  Arith Logic IMM ";
#endif
    uint32_t alu_op_sel_shift = ((get_funct7_b5()) << 3) | get_funct3();
    uint32_t alu_op_sel = ((get_funct3() & 0x3) == 1) ? alu_op_sel_shift : 
                                                        get_funct3();
    write_rf(get_rd(), 
        (this->*alu_op_map[alu_op_sel])(rf[get_rs1()], get_imm_i()));
    next_pc = pc + 4;
}

void core::load() {
#ifdef PRINT_EXEC
    std::cout << "  Load ";
#endif
    write_rf(get_rd(),
        (this->*load_op_map[get_funct3()])(rf[get_rs1()]+get_imm_i()));
    next_pc = pc + 4;
}

void core::store() {
#ifdef PRINT_EXEC
    std::cout << "  Store ";
#endif
    (this->*store_op_map[get_funct3()])(rf[get_rs1()]+get_imm_s(),
                                        rf[get_rs2()]);
    next_pc = pc + 4;
}

void core::branch() {
#ifdef PRINT_EXEC
    std::cout << "  Branch ";
#endif
    uint32_t alu_op_sel = get_funct3();
    if ((this->*branch_op_map[alu_op_sel])())
        next_pc = pc + get_imm_b();
    else
        next_pc = pc + 4;
}

void core::jalr() {
#ifdef PRINT_EXEC
    std::cout << "  JALR ";
#endif
    next_pc = (rf[get_rs1()] + get_imm_i()) & 0xFFFFFFFE;
    write_rf(get_rd(), pc + 4);
}

void core::jal() {
#ifdef PRINT_EXEC
    std::cout << "  JAL ";
#endif
    write_rf(get_rd(), pc + 4);
    next_pc = pc + get_imm_j();
}

void core::lui() {
#ifdef PRINT_EXEC
    std::cout << "  LUI ";
#endif
    write_rf(get_rd(), get_imm_u());
    next_pc = pc + 4;
}

void core::auipc() {
#ifdef PRINT_EXEC
    std::cout << "  AUIPC ";
#endif
    write_rf(get_rd(), get_imm_u() + pc);
    next_pc = pc + 4;
}

void core::system() {
#ifdef PRINT_EXEC
    std::cout << "  System ";
#endif
    if (inst == INST_ECALL or inst == INST_EBREAK) {
        running = false;
    } else {
        csr_exists();
        auto csr_op = csr_op_map.find(get_funct3());
        if (csr_op != csr_op_map.end()) (this->*csr_op->second)();
        else unsupported();
        next_pc = pc + 4;
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
void core::csr_exists() {
    uint32_t csr_address = get_csr_addr();
    auto it = csr.find(csr_address);
    if (it == csr.end()) {
        std::cerr << "Unsupported CSR. Address: " << std::hex << csr_address 
                  << std::dec <<std::endl;
        throw std::runtime_error("Unsupported CSR");
    }
}

void core::csr_read_write() {
    // using temp in case rd and rs1 are the same register
    uint32_t init_val_rs1 = rf[get_rs1()];
    write_rf(get_rd(), (uint32_t)csr[get_csr_addr()]);
    write_csr(get_csr_addr(), init_val_rs1);
}

void core::csr_read_set() {
    uint32_t init_val_rs1 = rf[get_rs1()];
    write_rf(get_rd(), (uint32_t)csr[get_csr_addr()]);
    write_csr(get_csr_addr(), csr[get_csr_addr()] | init_val_rs1);
}

void core::csr_read_clear() {
    uint32_t init_val_rs1 = rf[get_rs1()];
    write_rf(get_rd(), (uint32_t)csr[get_csr_addr()]);
    write_csr(get_csr_addr(), csr[get_csr_addr()] & ~init_val_rs1);
}

void core::csr_read_write_imm(){
    write_rf(get_rd(), (uint32_t)csr[get_csr_addr()]);
    write_csr(get_csr_addr(), get_uimm_csr());
}

void core::csr_read_set_imm(){
    write_rf(get_rd(), (uint32_t)csr[get_csr_addr()]);
    write_csr(get_csr_addr(), csr[get_csr_addr()] | get_uimm_csr());
}

void core::csr_read_clear_imm(){
    write_rf(get_rd(), (uint32_t)csr[get_csr_addr()]);
    write_csr(get_csr_addr(), csr[get_csr_addr()] & ~get_uimm_csr());
}

/*
* Utilities
*/
void core::dump() {
    std::cout << std::dec << "Inst Counter: " << inst_cnt << std::endl;
    std::cout << "PC: " << MEM_ADDR_FORMAT(pc) << std::endl;
    for(uint32_t i = 0; i < 32; i+=4){
        for(uint32_t j = 0; j < 4; j++) {
            std::cout << FRF(i+j, rf[i+j]) << " ";
        }
        std::cout << std::endl;
    }
    // iterate over CSR map and print addresses and values as hex
    for (auto it = csr.begin(); it != csr.end(); it++)
        std::cout << CSR_DEF(it->first, it->second) << std::endl;
    
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
         << std::hex << csr[CSR_MSCRATCH] << std::endl;
}

/*
* Immediate extraction
*/
uint32_t core::get_opcode() { return (inst & M_OPC7); }
uint32_t core::get_funct7() { return (inst & M_FUNCT7) >> 25; }
uint32_t core::get_funct7_b5() { return (inst & M_FUNCT7_B5) >> 30; }
uint32_t core::get_funct3() { return (inst & M_FUNCT3) >> 12; }
uint32_t core::get_rd() { return (inst & M_RD) >> 7; }
uint32_t core::get_rs1() { return (inst & M_RS1) >> 15; }
uint32_t core::get_rs2() { return (inst & M_RS2) >> 20; }
uint32_t core::get_imm_i() { return int32_t(inst & M_IMM_31_20) >> 20; }
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

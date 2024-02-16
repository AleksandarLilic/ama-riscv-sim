#include "core.h"

core::core(uint32_t base_address, memory *mem) {
    pc = base_address;
    next_pc = 0;
    this->mem = mem;
    for (uint32_t i = 0; i < 32; i++) rf[i] = 0;
    // instruction decoders
    decoder_map[(uint8_t)opcode::al_reg] = &core::al_reg;
    decoder_map[(uint8_t)opcode::al_imm] = &core::al_imm;
    decoder_map[(uint8_t)opcode::load] = &core::load;
    decoder_map[(uint8_t)opcode::store] = &core::store;
    decoder_map[(uint8_t)opcode::branch] = &core::branch;
    decoder_map[(uint8_t)opcode::jalr] = &core::jalr;
    decoder_map[(uint8_t)opcode::jal] = &core::jal;
    decoder_map[(uint8_t)opcode::lui] = &core::lui;
    decoder_map[(uint8_t)opcode::auipc] = &core::auipc;
    decoder_map[(uint8_t)opcode::system] = &core::system;
    // alu operations
    alu_map[(uint8_t)alu_op_t::op_add] = &core::al_add;
    alu_map[(uint8_t)alu_op_t::op_sub] = &core::al_sub;
    alu_map[(uint8_t)alu_op_t::op_sll] = &core::al_sll;
    alu_map[(uint8_t)alu_op_t::op_srl] = &core::al_srl;
    alu_map[(uint8_t)alu_op_t::op_sra] = &core::al_sra;
    alu_map[(uint8_t)alu_op_t::op_slt] = &core::al_slt;
    alu_map[(uint8_t)alu_op_t::op_sltu] = &core::al_sltu;
    alu_map[(uint8_t)alu_op_t::op_xor] = &core::al_xor;
    alu_map[(uint8_t)alu_op_t::op_or] = &core::al_or;
    alu_map[(uint8_t)alu_op_t::op_and] = &core::al_and;
    // load operations
    load_map[(uint8_t)load_op_t::op_byte] = &core::load_byte;
    load_map[(uint8_t)load_op_t::op_half] = &core::load_half;
    load_map[(uint8_t)load_op_t::op_word] = &core::load_word;
    load_map[(uint8_t)load_op_t::op_byte_u] = &core::load_byte_u;
    load_map[(uint8_t)load_op_t::op_half_u] = &core::load_half_u;
    // store operations
    store_map[(uint8_t)store_op_t::op_byte] = &core::store_byte;
    store_map[(uint8_t)store_op_t::op_half] = &core::store_half;
    store_map[(uint8_t)store_op_t::op_word] = &core::store_word;
    // branch operations
    branch_map[(uint8_t)branch_op_t::op_beq] = &core::branch_eq;
    branch_map[(uint8_t)branch_op_t::op_bne] = &core::branch_ne;
    branch_map[(uint8_t)branch_op_t::op_blt] = &core::branch_lt;
    branch_map[(uint8_t)branch_op_t::op_bge] = &core::branch_ge;
    branch_map[(uint8_t)branch_op_t::op_bltu] = &core::branch_ltu;
    branch_map[(uint8_t)branch_op_t::op_bgeu] = &core::branch_geu;
}

void core::exec() {
    inst = mem->rd32(pc);
    std::cout << MEM_ADDR_FORMAT(pc) << " : " << std::setw(8) 
              << std::setfill('0') << std::hex << inst << std::dec;
    auto inst_dec = decoder_map.find(get_opcode());
    if (inst_dec != decoder_map.end()) (this->*inst_dec->second)();
    else unsupported();
    pc = next_pc;
    std::cout << std::endl;
}

void core::al_reg() {
    std::cout << "  Arith Logic REG ";
    uint32_t alu_op_sel = ((get_funct7_b5()) << 3) | get_funct3();
    write_rf(get_rd(), 
        (this->*alu_map[alu_op_sel])(rf[get_rs1()], rf[get_rs2()]));
    next_pc = pc + 4;
}

void core::al_imm() {
    std::cout << "  Arith Logic IMM";
    uint32_t alu_op_sel_shift = ((get_funct7_b5()) << 3) | get_funct3();
    uint32_t alu_op_sel = ((get_funct3() & 0x3) == 1) ? alu_op_sel_shift : 
                                                        get_funct3();
    write_rf(get_rd(), 
        (this->*alu_map[alu_op_sel])(rf[get_rs1()], get_imm_i()));
    next_pc = pc + 4;
}

void core::load() {
    std::cout << "  LOAD";
    write_rf(get_rd(),
        (this->*load_map[get_funct3()])(rf[get_rs1()]+get_imm_i()));
    next_pc = pc + 4;
}

void core::store() {
    std::cout << "  STORE";
    (this->*store_map[get_funct3()])(rf[get_rs1()]+get_imm_s(), rf[get_rs2()]);
    next_pc = pc + 4;
}

void core::branch() {
    std::cout << "  BRANCH";
    uint32_t alu_op_sel = get_funct3();
    if ((this->*branch_map[alu_op_sel])())
        next_pc = pc + get_imm_b();
    else
        next_pc = pc + 4;
}

void core::jalr() {
    std::cout << "  JALR";
    next_pc = (rf[get_rs1()] + get_imm_i()) & 0xFFFFFFFE;
    write_rf(get_rd(), pc + 4);
}

void core::jal() {
    std::cout << "  JAL";
    write_rf(get_rd(), pc + 4);
    next_pc = pc + get_imm_j();
}

void core::lui() {
    std::cout << "  LUI";
    write_rf(get_rd(), get_imm_u());
    next_pc = pc + 4;
}

void core::auipc() {
    std::cout << "  AUIPC";
    write_rf(get_rd(), get_imm_u() + pc);
    next_pc = pc + 4;
}

void core::system() {
    // TODO
    std::cout << "  SYSTEM";
    next_pc = pc + 4;
}

void core::unsupported() {
    // TODO
    std::cerr << "Unsupported" << std::endl;
    next_pc = pc + 4;
}

void core::reset() {
    // TODO
}

void core::dump() {
    for(uint32_t i = 0; i < 32; i+=4){
        for(uint32_t j = 0; j < 4; j++) {
            std::cout << FRF(i+j, rf[i+j]) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;
}

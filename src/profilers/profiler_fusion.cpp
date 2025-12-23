#include "profiler_fusion.h"

void profiler_fusion::attack(inst_opt opt) {
    if (!active) return;
    this->opt = opt;
    do transition();
    while (current_state != state::START);
}

void profiler_fusion::transition() {
    switch (current_state) {
        case state::START:
            if (opt.trig == trigger::slli_lea) current_state = state::LEA_1;
            // TODO: support for other fusion options
            break;

        case state::LEA_1:
            //std::cout << "found slli " << opt.rvc << std::endl;
            current_state = lea_1();
            break;

        case state::LEA_2:
            //std::cout << "    found addi imm opt range" << std::endl;
            current_state = lea_2();
            break;

        case state::LEA_MATCH:
            //std::cout << "        found lea fusion" << std::endl;
            lea_match_cnt++;
            current_state = state::START;
            break;

        default:
            break;
    }
}

state profiler_fusion::lea_1() {
    ip.set(opt.inst);
    uint32_t imm = opt.rvc ? ip.c_imm_slli() : ip.imm_i_shamt();
    // 2- and 4-byte shifts, rv64 should add 8-byte shift (imm < 4)
    if ((imm > 0) && (imm < 3)) return state::LEA_2;
    return state::START;
}

state profiler_fusion::lea_2() {
    uint32_t rd = ip.rd(); // rd of the first instruction (slli)
    ip.set(opt.inst_nx);
    // technically possible (though highly unlikely) for second instruction
    // to be the opposite length of the first, so check both c.addi and addi

    // RVC addi
    uint32_t funct4 = ip.c_funct4();
    uint32_t funct3 = ip.c_funct3();
    bool inst_system = (ip.c_rs2() == 0x0 && ip.rd() == 0x0);
    if (funct3 == 0x4 && funct4 == 0x9 && !inst_system) {
        if (ip.c_rs2() == rd) return state::LEA_MATCH;
    }

    // RV32I addi
    uint32_t alu_op_sel = ((ip.funct7_b5()) << 3) | ip.funct3();
    if (ip.opcode() == TO_U8(opcode::alu_reg) &&
        alu_op_sel == TO_U8(alu_r_op_t::op_add)) {
        if ((ip.rd() == ip.rs1()) && (ip.rs2() == rd)) return state::LEA_MATCH;
    }

    return state::START;
}

// FIXME: should be passed to the main profiler and stored in the output file
void profiler_fusion::finish(bool silent) {
    #ifdef DPI
    return;
    #endif

    if (silent) return;

    std::cout << "Profiler - Fusion:\n"
              << INDENT << "LEA opportunities: " << lea_match_cnt << "\n";
}

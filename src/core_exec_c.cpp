#include "defines.h"
#include "core.h"

// arithmetic and logic operations
void core::c_addi() {
    uint32_t res = alu_addi(rf[ip.rd()], ip.c_imm_arith());
    write_rf(ip.rd(), res);
    DASM_OP(c.addi)
    PROF_G(c_addi)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_RD << "," << TO_I32(ip.c_imm_arith());
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_li() {
    uint32_t res = ip.c_imm_arith();
    write_rf(ip.rd(), res);
    DASM_OP(c.li)
    PROF_G(c_li)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_RD << "," << TO_I32(ip.c_imm_arith());
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_lui() {
    uint32_t res = ip.c_imm_lui();
    if (res == 0) tu.e_illegal_inst("c.lui (imm=0)", 4);
    write_rf(ip.rd(), res);
    DASM_OP(c.lui)
    PROF_G(c_lui)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_RD << "," << FHEXN((ip.c_imm_lui() >> 12), 5);
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_nop() {
    // write_rf(0, alu_addi(rf[0], 0));
    DASM_OP(c.nop)
    PROF_G(c_nop)
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op;
    #endif
    next_pc = pc + 2;
}

void core::c_addi16sp() {
    if (ip.c_imm_16sp() == 0) tu.e_illegal_inst("c.addi16sp (imm=0)", 4);
    uint32_t res = alu_addi(rf[2], ip.c_imm_16sp());
    write_rf(2, res);
    DASM_OP(c.addi16sp)
    PROF_G(c_addi16sp)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_RD << "," << TO_I32(ip.c_imm_16sp());
    DASM_RD_UPDATE_P(2);
    #endif
    next_pc = pc + 2;
}

void core::c_srli() {
    uint32_t res = alu_srli(rf[ip.c_regh()], ip.c_imm_arith());
    write_rf(ip.c_regh(), res);
    DASM_OP(c.srli)
    PROF_G(c_srli)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << TO_I32(ip.c_imm_arith());
    DASM_RD_UPDATE_P(ip.c_regh());
    #endif
    next_pc = pc + 2;
}

void core::c_srai() {
    uint32_t res = alu_srai(rf[ip.c_regh()], ip.c_imm_arith());
    write_rf(ip.c_regh(), res);
    DASM_OP(c.srai)
    PROF_G(c_srai)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << TO_I32(ip.c_imm_arith());
    DASM_RD_UPDATE_P(ip.c_regh());
    #endif
    next_pc = pc + 2;
}

void core::c_andi() {
    uint32_t res = alu_andi(rf[ip.c_regh()], ip.c_imm_arith());
    write_rf(ip.c_regh(), res);
    DASM_OP(c.andi)
    PROF_G(c_andi)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << TO_I32(ip.c_imm_arith());
    DASM_RD_UPDATE_P(ip.c_regh());
    #endif
    next_pc = pc + 2;
}

void core::c_and() {
    uint32_t res = alu_and(rf[ip.c_regh()], rf[ip.c_regl()]);
    write_rf(ip.c_regh(), res);
    DASM_OP(c.and)
    PROF_G(c_and)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.c_regh());
    #endif
    next_pc = pc + 2;
}

void core::c_or() {
    uint32_t res = alu_or(rf[ip.c_regh()], rf[ip.c_regl()]);
    write_rf(ip.c_regh(), res);
    DASM_OP(c.or)
    PROF_G(c_or)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.c_regh());
    #endif
    next_pc = pc + 2;
}

void core::c_xor() {
    uint32_t res = alu_xor(rf[ip.c_regh()], rf[ip.c_regl()]);
    write_rf(ip.c_regh(), res);
    DASM_OP(c.xor)
    PROF_G(c_xor)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.c_regh());
    #endif
    next_pc = pc + 2;
}

void core::c_sub() {
    uint32_t res = alu_sub(rf[ip.c_regh()], rf[ip.c_regl()]);
    write_rf(ip.c_regh(), res);
    DASM_OP(c.sub)
    PROF_G(c_sub)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << DASM_CREGL;
    DASM_RD_UPDATE_P(ip.c_regh());
    #endif
    next_pc = pc + 2;
}

void core::c_addi4spn() {
    uint32_t res = alu_addi(rf[2], ip.c_imm_4spn());
    if (ip.c_imm_4spn() == 0) tu.e_illegal_inst("c.addi4spn (imm=0)", 4);
    if (inst == 0) tu.e_illegal_inst("c.inst == 0", 4);
    write_rf(ip.c_regl(), res);
    DASM_OP(c.addi4spn)
    PROF_G(c_addi4spn)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << DASM_CREGL << ",x2,"
                << TO_I32(ip.c_imm_4spn());
    DASM_RD_UPDATE_P(ip.c_regl());
    #endif
    next_pc = pc + 2;
}

void core::c_slli() {
    uint32_t res = alu_sll(rf[ip.rd()], ip.c_imm_slli());
    write_rf(ip.rd(), res);
    DASM_OP(c.slli)
    PROF_G(c_slli)
    PROF_SPARSITY_ALU
    #ifdef PROFILERS_EN
    prof_fusion.attack({trigger::slli_lea, inst, mem->just_inst(pc + 2), true});
    #endif
    #ifdef DASM_EN
    DASM_OP_RD << "," << FHEXN(TO_I32(ip.c_imm_slli()), 2);
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_mv() {
    uint32_t res = rf[ip.c_rs2()];
    write_rf(ip.rd(), res);
    DASM_OP(c.mv)
    PROF_G(c_mv)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_RD << "," << rf_names[ip.c_rs2()][rf_names_idx];
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

void core::c_add() {
    uint32_t res = alu_add(rf[ip.rd()], rf[ip.c_rs2()]);
    write_rf(ip.rd(), res);
    DASM_OP(c.add)
    PROF_G(c_add)
    PROF_SPARSITY_ALU
    #ifdef DASM_EN
    DASM_OP_RD << "," << rf_names[ip.c_rs2()][rf_names_idx];
    DASM_RD_UPDATE;
    #endif
    next_pc = pc + 2;
}

// memory operations
void core::c_lw() {
    uint32_t rs1 = rf[ip.c_regh()];
    uint32_t loaded = mem->rd(rs1 + ip.c_imm_mem(), 4u);
    if (tu.is_trapped()) return;
    write_rf(ip.c_regl(), loaded);
    DASM_OP(c.lw)
    PROF_G(c_lw)
    PROF_SPARSITY_MEM_L
    #ifdef PROFILERS_EN
    prof.log_stack_access_load((rs1 + ip.c_imm_mem()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << DASM_CREGL << "," << TO_I32(ip.c_imm_mem())
                << "(" << rf_names[ip.c_regh()][rf_names_idx] << ")";
    DASM_RD_UPDATE_P(ip.c_regl());
    if (ip.rd()) {
        dasm.asm_ss << " <- mem["
                    << MEM_ADDR_FORMAT(TO_I32(ip.c_imm_mem()) + rs1) << "]";
    }
    #endif
    next_pc = pc + 2;
}

void core::c_lwsp() {
    uint32_t loaded = mem->rd(rf[2] + ip.c_imm_lwsp(), 4u);
    if (tu.is_trapped()) return;
    write_rf(ip.rd(), loaded);
    DASM_OP(c.lwsp)
    PROF_G(c_lwsp)
    PROF_SPARSITY_MEM_L
    #ifdef PROFILERS_EN
    prof.log_stack_access_load((rf[2] + ip.c_imm_lwsp()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef DASM_EN
    DASM_OP_RD << "," << TO_I32(ip.c_imm_lwsp())
               << "(" << rf_names[2][rf_names_idx] << ")";
    DASM_RD_UPDATE;
    if (ip.rd()) {
        dasm.asm_ss << " <- mem["
                    << MEM_ADDR_FORMAT(TO_I32(ip.c_imm_lwsp()) + rf[2]) << "]";
    }
    #endif
    next_pc = pc + 2;
}

void core::c_sw() {
    uint32_t val = (rf[ip.c_regh()] + ip.c_imm_mem());
    mem->wr(val, rf[ip.c_regl()], 4u);
    if (tu.is_trapped()) return;
    DASM_OP(c.sw)
    PROF_G(c_sw)
    PROF_SPARSITY_MEM_S
    #ifdef PROFILERS_EN
    prof.log_stack_access_store(
        (rf[ip.c_regh()] + ip.c_imm_mem()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << DASM_CREGL << "," << TO_I32(ip.c_imm_mem())
                << "(" << rf_names[ip.c_regh()][rf_names_idx] << ")";
    DASM_MEM_UPDATE_P(TO_I32(ip.c_imm_mem()) + rf[ip.c_regh()], ip.c_regl());
    #endif
    next_pc = pc + 2;
}

void core::c_swsp() {
    uint32_t val = (rf[2] + ip.c_imm_swsp());
    mem->wr(val, rf[ip.c_rs2()], 4u);
    if (tu.is_trapped()) return;
    DASM_OP(c.swsp)
    PROF_G(c_swsp)
    PROF_SPARSITY_MEM_S
    #ifdef PROFILERS_EN
    prof.log_stack_access_store((rf[2] + ip.c_imm_swsp()) > TO_U32(rf[2]));
    prof_perf.set_perf_event_flag(perf_event_t::mem);
    #endif
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << rf_names[ip.c_rs2()][rf_names_idx] << ","
                << TO_I32(ip.c_imm_swsp())
                << "(" << rf_names[2][rf_names_idx] << ")";
    DASM_MEM_UPDATE_P(TO_I32(ip.c_imm_swsp()) + rf[2], ip.c_rs2());
    #endif
    next_pc = pc + 2;
}

// control transfer operations
void core::c_beqz() {
    uint32_t target_pc = (pc + ip.c_imm_b());
    if (rf[ip.c_regh()] == 0) {
        next_pc = target_pc;
        PROF_B_T(c_beqz)
    } else {
        next_pc = pc + 2;
        PROF_B_NT(c_beqz, target_pc)
    }
    #ifdef PROFILERS_EN
    branch_taken = (next_pc != (pc + 2));
    prof_perf.update_branch(next_pc, branch_taken);
    prof_perf.set_perf_event_flag(perf_event_t::branch);
    #endif
    DASM_OP(c.beqz)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << std::hex << pc + TO_I32(ip.c_imm_b()) << std::dec;
    #endif
}

void core::c_bnez() {
    uint32_t target_pc = (pc + ip.c_imm_b());
    if (rf[ip.c_regh()] != 0) {
        next_pc = target_pc;
        PROF_B_T(c_bnez)
    } else {
        next_pc = pc + 2;
        PROF_B_NT(c_beqz, target_pc)
    }
    #ifdef PROFILERS_EN
    branch_taken = (next_pc != (pc + 2));
    prof_perf.update_branch(next_pc, branch_taken);
    prof_perf.set_perf_event_flag(perf_event_t::branch);
    #endif
    DASM_OP(c.bnez)
    #ifdef DASM_EN
    DASM_OP_CREGH << "," << std::hex << pc + TO_I32(ip.c_imm_b()) << std::dec;
    #endif
}

void core::c_j() {
    next_pc = pc + ip.c_imm_j();
    DASM_OP(c.j)
    PROF_J(c_j)
    #ifdef PROFILERS_EN
    prof_perf.update_jal(next_pc, true, false);
    branch_taken = true;
    #endif
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << std::hex << pc + TO_I32(ip.c_imm_j())
                << std::dec;
    #endif
}

void core::c_jal() {
    next_pc = pc + ip.c_imm_j();
    write_rf(1, pc + 2);
    DASM_OP(c.jal)
    PROF_J(c_jal)
    #ifdef PROFILERS_EN
    prof_perf.update_jal(next_pc, false, false);
    branch_taken = true;
    #endif
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op << " " << std::hex << pc + TO_I32(ip.c_imm_j())
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

    #ifdef PROFILERS_EN
    bool ret_inst = (inst == INST_C_RET);
    prof_perf.update_jalr(next_pc, ret_inst, true, pc + 2); // no ra, tail calls
    branch_taken = true;
    #endif

    #ifdef DASM_EN
    DASM_OP_RD;
    if (ret_inst) dasm.asm_ss << " # ret";
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

// system
void core::c_ebreak() {
    running = false;
    DASM_OP(c.ebreak)
    PROF_G(c_ebreak)
    #ifdef DASM_EN
    dasm.asm_ss << dasm.op;
    #endif
}

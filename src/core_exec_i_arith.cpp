#include "defines.h"
#include "core.h"

// register
uint32_t core::alu_add(uint32_t a, uint32_t b) { return a + b; }
uint32_t core::alu_sub(uint32_t a, uint32_t b) { return a - b; }
uint32_t core::alu_sll(uint32_t a, uint32_t b) { return a << b; }
uint32_t core::alu_srl(uint32_t a, uint32_t b) { return a >> b; }
uint32_t core::alu_sra(uint32_t a, uint32_t b) { return TO_I32(a) >> b; }
uint32_t core::alu_slt(uint32_t a, uint32_t b) { return TO_I32(a) < TO_I32(b); }
uint32_t core::alu_sltu(uint32_t a, uint32_t b) { return a < b; }
uint32_t core::alu_xor(uint32_t a, uint32_t b) { return a ^ b; }
uint32_t core::alu_or(uint32_t a, uint32_t b) { return a | b; }
uint32_t core::alu_and(uint32_t a, uint32_t b) { return a & b; }

// immediates
uint32_t core::alu_addi(uint32_t a, uint32_t b) { return alu_add(a, b); }
uint32_t core::alu_slli(uint32_t a, uint32_t b) {
    #ifdef PROFILERS_EN
    prof_fusion.attack(
        {trigger::slli_lea, inst, mem->just_inst(pc + 4), false}
    );
    #endif
    return alu_sll(a, b);
}
uint32_t core::alu_srli(uint32_t a, uint32_t b) { return alu_srl(a, b); }
uint32_t core::alu_srai(uint32_t a, uint32_t b) { return alu_sra(a, b); }
uint32_t core::alu_slti(uint32_t a, uint32_t b) {
    #ifdef PROFILERS_EN
    if (inst == INST_HINT_LOG_START) {
        prof_state(prof_pc.should_start());
        return 0;
    } else if (inst == INST_HINT_LOG_END) {
        prof_state(false);
        running = !prof_pc.should_exit();
        return 0;
    }
    #endif
    return alu_slt(a, b);
}
uint32_t core::alu_sltiu(uint32_t a, uint32_t b) { return alu_sltu(a, b); }
uint32_t core::alu_xori(uint32_t a, uint32_t b) { return alu_xor(a, b); }
uint32_t core::alu_ori(uint32_t a, uint32_t b) { return alu_or(a, b); }
uint32_t core::alu_andi(uint32_t a, uint32_t b) { return alu_and(a, b); }

// upper
void core::lui() { write_rf(ip.rd(), ip.imm_u()); }
void core::auipc() { write_rf(ip.rd(), ip.imm_u() + pc); }

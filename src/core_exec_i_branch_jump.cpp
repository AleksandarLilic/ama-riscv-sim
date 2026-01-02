#include "defines.h"
#include "core.h"

bool core::branch_beq() {
    return rf[ip.rs1()] == rf[ip.rs2()];
}
bool core::branch_bne() {
    return rf[ip.rs1()] != rf[ip.rs2()];
}
bool core::branch_blt() {
    return TO_I32(rf[ip.rs1()]) < TO_I32(rf[ip.rs2()]);
}
bool core::branch_bge() {
    return TO_I32(rf[ip.rs1()]) >= TO_I32(rf[ip.rs2()]);
}
bool core::branch_bltu() {
    return TO_U32(rf[ip.rs1()]) < TO_U32(rf[ip.rs2()]);
}
bool core::branch_bgeu() {
    return TO_U32(rf[ip.rs1()]) >= TO_U32(rf[ip.rs2()]);
}

void core::jalr() {
    if (ip.funct3() != 0) tu.e_unsupported_inst("jalr, funct3 != 0");
    next_pc = ((rf[ip.rs1()] + ip.imm_i()) & 0xFFFFFFFE);

    #ifndef RV32C
    bool address_unaligned = (next_pc % 4 != 0);
    if (address_unaligned) {
        tu.e_inst_addr_misaligned(next_pc, "jalr unaligned access");
        return;
    }
    #endif
    uint32_t ra = (pc + 4);
    write_rf(ip.rd(), ra);
}

void core::jal() {
    next_pc = (pc + ip.imm_j());
    #ifndef RV32C
    bool address_unaligned = (next_pc % 4 != 0);
    if (address_unaligned) {
        tu.e_inst_addr_misaligned(next_pc, "jal unaligned access");
        return;
    }
    #endif
    uint32_t ra = (pc + 4);
    write_rf(ip.rd(), ra);
}

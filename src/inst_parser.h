#pragma once

#include "defines.h"

class inst_parser {
    public:
        uint32_t copcode() { return (inst & M_OPC2); }
        uint32_t opcode() { return (inst & M_OPC7); }
        //uint32_t funct7() { return (inst & M_FUNCT7) >> 25; }
        uint32_t funct7_b5() { return (inst & M_FUNCT7_B5) >> 30; }
        uint32_t funct7_b1() { return (inst & M_FUNCT7_B1) >> 25; }
        uint32_t funct3() { return (inst & M_FUNCT3) >> 12; }
        uint32_t cfunct2h() { return (inst & M_CFUNCT2H) >> 10; }
        uint32_t cfunct2l() { return (inst & M_CFUNCT2L) >> 5; }
        uint32_t cfunct3() { return (inst & M_CFUNCT3) >> 13; }
        uint32_t cfunct4() { return (inst & M_CFUNCT4) >> 12; }
        uint32_t cfunct6() { return (inst & M_CFUNCT6) >> 10; }
        uint32_t rd() { return (inst & M_RD) >> 7; }
        uint32_t rs1() { return (inst & M_RS1) >> 15; }
        uint32_t rs2() { return (inst & M_RS2) >> 20; }
        uint32_t crs2() { return (inst & M_CRS2) >> 2; }
        uint32_t cregh() { return (0x8 | (inst & M_CREGH) >> 7); }
        uint32_t cregl() { return (0x8 | (inst & M_CREGL) >> 2); }
        uint32_t imm_i() { return int32_t(inst & M_IMM_31_20) >> 20; }
        uint32_t imm_i_shamt() { return (inst & M_IMM_24_20) >> 20; }
        uint32_t csr_addr() { return (inst & M_IMM_31_20) >> 20; }
        uint32_t uimm_csr() { return rs1(); }
        uint32_t imm_s() {
            return ((int32_t(inst) & M_IMM_31_25) >> 20) |
                ((inst & M_IMM_11_8) >> 7) |
                ((inst & M_IMM_7) >> 7);
        }

        uint32_t imm_b() {
            return ((int32_t(inst) & M_IMM_31) >> 19) |
                ((inst & M_IMM_7) << 4) |
                ((inst & M_IMM_30_25) >> 20) |
                ((inst & M_IMM_11_8) >> 7);
        }

        uint32_t imm_j() {
            return ((int32_t(inst) & M_IMM_31) >> 11) |
                ((inst & M_IMM_19_12)) |
                ((inst & M_IMM_20) >> 9) |
                ((inst & M_IMM_30_25) >> 20) |
                ((inst & M_IMM_24_21) >> 20);
        }

        uint32_t imm_u() {
            return ((int32_t(inst) & M_IMM_31_20)) | ((inst & M_IMM_19_12));
        }

        uint32_t imm_c_arith() {
            int16_t imm_5 = (TO_I16(inst << (15-12)) >> (15-5)) & 0xffe0;
            int16_t imm_4_0 = TO_I16((inst & M_IMM_6_2) >> 2);
            return imm_5 | imm_4_0;
        }

        uint32_t imm_c_slli() {
            uint16_t imm_5 = (inst & M_IMM_12) >> 7;
            uint16_t imm_4_0 = (inst & M_IMM_6_2) >> 2;
            return imm_5 | imm_4_0;
        }

        uint32_t imm_c_lui() {
            int32_t imm_17 = (TO_I32(inst << (31-12)) >> (31-17)) & 0xfffe0000;
            int32_t imm_16_12 = (TO_I32(inst & M_IMM_6_2) << 10);
            return imm_17 | imm_16_12;
        }

        uint32_t imm_c_16sp() {
            int16_t imm_9 = (TO_I16(inst << (15-12)) >> (15-9)) & 0xfe00;
            int16_t imm_8_7 = (TO_I16(inst & M_IMM_4_3) << 4);
            int16_t imm_6 = (TO_I16(inst & M_IMM_5) << 1);
            int16_t imm_5 = (TO_I16(inst & M_IMM_2) << 3);
            int16_t imm_4 = (TO_I16(inst & M_IMM_6) >> 2);
            return imm_9 | imm_8_7 | imm_6 | imm_5 | imm_4;
        }

        uint32_t imm_c_b() {
            int16_t imm_8 = (TO_I16(inst << (15-12)) >> (15-8)) & 0xff00;
            int16_t imm_7_6 = (TO_I16(inst & M_IMM_6_5) << 1);
            int16_t imm_5 = (TO_I16(inst & M_IMM_2) << 3);
            int16_t imm_4_3 = (TO_I16(inst & M_IMM_11_10) >> 7);
            int16_t imm_2_1 = (TO_I16(inst & M_IMM_4_3) >> 2);
            return imm_8 | imm_7_6 | imm_5 | imm_4_3 | imm_2_1;
        }

        uint32_t imm_c_j() {
            int16_t imm_11 = (TO_I16(inst << (15-12)) >> (15-11)) & 0xf800;
            int16_t imm_10 = (TO_I16(inst & M_IMM_8) << 2);
            int16_t imm_9_8 = (TO_I16(inst & M_IMM_10_9) >> 1);
            int16_t imm_7 = (TO_I16(inst & M_IMM_6) << 1);
            int16_t imm_6 = (TO_I16(inst & M_IMM_7) >> 1);
            int16_t imm_5 = (TO_I16(inst & M_IMM_2) << 3);
            int16_t imm_4 = (TO_I16(inst & M_IMM_11) >> 7);
            int16_t imm_3_1 = (TO_I16(inst & M_IMM_5_3) >> 2);
            return imm_11 | imm_10 | imm_9_8 | imm_7 | imm_6 | imm_5 | imm_4
                   | imm_3_1;
        }

        uint32_t imm_c_4spn() {
            uint16_t imm_9_6 = (inst & M_IMM_10_7) >> 1;
            uint16_t imm_5_4 = (inst & M_IMM_12_11) >> 7;
            uint16_t imm_3 = (inst & M_IMM_5) >> 2;
            uint16_t imm_2 = (inst & M_IMM_6) >> 4;
            return imm_9_6 | imm_5_4 | imm_3 | imm_2;
        }

        uint32_t imm_c_mem() {
            uint8_t imm_6 = (inst & M_IMM_5) << 1;
            uint8_t imm_5_3 = (inst & M_IMM_12_10) >> 7;
            uint8_t imm_2 = (inst & M_IMM_6) >> 4;
            return imm_6 | imm_5_3 | imm_2;
        }

        uint32_t imm_c_lwsp() {
            uint8_t imm_7_6 = (inst & M_IMM_3_2) << 4;
            uint8_t imm_5 = (inst & M_IMM_12) >> 7;
            uint8_t imm_4_2 = (inst & M_IMM_6_4) >> 2;
            return imm_7_6 | imm_5 | imm_4_2;
        }

        uint32_t imm_c_swsp() {
            uint8_t imm_7_6 = (inst & M_IMM_8_7) >> 1;
            uint8_t imm_5_2 = (inst & M_IMM_12_9) >> 7;
            return imm_7_6 | imm_5_2;
        }

    public:
        uint32_t inst;
};

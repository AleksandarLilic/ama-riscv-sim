#pragma once

#include "defines.h"
#include "memory.h"
#include "profiler.h"

class core{
    public:
        core() = delete;
        core(uint32_t base_addr, memory *mem, std::string log_name);
        void exec();
        void exec_inst();
        void dump();
        std::string dump_state();
        void finish(bool dump_regs);
        uint32_t get_pc() { return pc; }
        uint32_t get_inst() { return inst; }
        uint32_t get_reg(uint32_t reg) { return rf[reg]; }
        uint32_t get_inst_cnt() { return inst_cnt; }
        #if defined(ENABLE_DASM) || defined(ENABLE_PROF)
        uint8_t inst_w = 8;
        #endif
        #ifdef ENABLE_DASM
        std::string get_inst_asm() { return dasm.asm_str; }
        #endif

    private:
        void write_rf(uint32_t reg, uint32_t data) { if(reg) rf[reg] = data; }
        void write_csr(uint16_t addr, uint32_t data) {
            if (csr.at(addr).perm == perm_t::ro)
                illegal("CSR write attempt to RO CSR", 4);
            if (csr.at(addr).perm == perm_t::warl_unimp)
                return;
            csr.at(addr).value = data;
        }
        void cntr_update();

        // instruction decoders
        void al_reg();
        void al_imm();
        void load();
        void store();
        void branch();
        void jalr();
        void jal();
        void lui();
        void auipc();
        void system();
        void misc_mem();
        void unsupported(const std::string &msg);
        void illegal(const std::string &msg, uint32_t memw);
        // void reset();

        // instruction parsing
        uint32_t get_copcode() { return (inst & M_OPC2); }
        uint32_t get_opcode() { return (inst & M_OPC7); }
        //uint32_t get_funct7() { return (inst & M_FUNCT7) >> 25; }
        uint32_t get_funct7_b5() { return (inst & M_FUNCT7_B5) >> 30; }
        uint32_t get_funct7_b1() { return (inst & M_FUNCT7_B1) >> 25; }
        uint32_t get_funct3() { return (inst & M_FUNCT3) >> 12; }
        uint32_t get_cfunct2h() { return (inst & M_CFUNCT2H) >> 10; }
        uint32_t get_cfunct2l() { return (inst & M_CFUNCT2L) >> 5; }
        uint32_t get_cfunct3() { return (inst & M_CFUNCT3) >> 13; }
        uint32_t get_cfunct4() { return (inst & M_CFUNCT4) >> 12; }
        uint32_t get_cfunct6() { return (inst & M_CFUNCT6) >> 10; }
        uint32_t get_rd() { return (inst & M_RD) >> 7; }
        uint32_t get_rs1() { return (inst & M_RS1) >> 15; }
        uint32_t get_rs2() { return (inst & M_RS2) >> 20; }
        uint32_t get_crs2() { return (inst & M_CRS2) >> 2; }
        uint32_t get_cregh() { return (0x8 | (inst & M_CREGH) >> 7); }
        uint32_t get_cregl() { return (0x8 | (inst & M_CREGL) >> 2); }
        uint32_t get_imm_i() { return int32_t(inst & M_IMM_31_20) >> 20; }
        uint32_t get_imm_i_shamt() { return (inst & M_IMM_24_20) >> 20; }
        uint32_t get_csr_addr() { return (inst & M_IMM_31_20) >> 20; }
        uint32_t get_uimm_csr() { return get_rs1(); }
        uint32_t get_imm_s() {
            return ((int32_t(inst) & M_IMM_31_25) >> 20) |
                ((inst & M_IMM_11_8) >> 7) |
                ((inst & M_IMM_7) >> 7);
        }

        uint32_t get_imm_b() {
            return ((int32_t(inst) & M_IMM_31) >> 19) |
                ((inst & M_IMM_7) << 4) |
                ((inst & M_IMM_30_25) >> 20) |
                ((inst & M_IMM_11_8) >> 7);
        }

        uint32_t get_imm_j() {
            return ((int32_t(inst) & M_IMM_31) >> 11) |
                ((inst & M_IMM_19_12)) |
                ((inst & M_IMM_20) >> 9) |
                ((inst & M_IMM_30_25) >> 20) |
                ((inst & M_IMM_24_21) >> 20);
        }

        uint32_t get_imm_u() {
            return ((int32_t(inst) & M_IMM_31_20)) |
                ((inst & M_IMM_19_12));
        }

        uint32_t get_imm_c_arith() {
            int16_t imm_5 = (TO_I16(inst << (15-12)) >> (15-5)) & 0xffe0;
            int16_t imm_4_0 = TO_I16((inst & M_IMM_6_2) >> 2);
            return imm_5 | imm_4_0;
        }

        uint32_t get_imm_c_slli() {
            uint16_t imm_5 = (inst & M_IMM_12) >> 7;
            uint16_t imm_4_0 = (inst & M_IMM_6_2) >> 2;
            return imm_5 | imm_4_0;
        }

        uint32_t get_imm_c_lui() {
            int32_t imm_17 = (TO_I32(inst << (31-12)) >> (31-17)) & 0xfffe0000;
            int32_t imm_16_12 = (TO_I32(inst & M_IMM_6_2) << 10);
            return imm_17 | imm_16_12;
        }

        uint32_t get_imm_c_16sp() {
            int16_t imm_9 = (TO_I16(inst << (15-12)) >> (15-9)) & 0xfe00;
            int16_t imm_8_7 = (TO_I16(inst & M_IMM_4_3) << 4);
            int16_t imm_6 = (TO_I16(inst & M_IMM_5) << 1);
            int16_t imm_5 = (TO_I16(inst & M_IMM_2) << 3);
            int16_t imm_4 = (TO_I16(inst & M_IMM_6) >> 2);
            return imm_9 | imm_8_7 | imm_6 | imm_5 | imm_4;
        }

        uint32_t get_imm_c_b() {
            int16_t imm_8 = (TO_I16(inst << (15-12)) >> (15-8)) & 0xff00;
            int16_t imm_7_6 = (TO_I16(inst & M_IMM_6_5) << 1);
            int16_t imm_5 = (TO_I16(inst & M_IMM_2) << 3);
            int16_t imm_4_3 = (TO_I16(inst & M_IMM_11_10) >> 7);
            int16_t imm_2_1 = (TO_I16(inst & M_IMM_4_3) >> 2);
            return imm_8 | imm_7_6 | imm_5 | imm_4_3 | imm_2_1;
        }

        uint32_t get_imm_c_j() {
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

        uint32_t get_imm_c_4spn() {
            uint16_t imm_9_6 = (inst & M_IMM_10_7) >> 1;
            uint16_t imm_5_4 = (inst & M_IMM_12_11) >> 7;
            uint16_t imm_3 = (inst & M_IMM_5) >> 2;
            uint16_t imm_2 = (inst & M_IMM_6) >> 4;
            return imm_9_6 | imm_5_4 | imm_3 | imm_2;
        }

        uint32_t get_imm_c_mem() {
            uint8_t imm_6 = (inst & M_IMM_5) << 1;
            uint8_t imm_5_3 = (inst & M_IMM_12_10) >> 7;
            uint8_t imm_2 = (inst & M_IMM_6) >> 4;
            return imm_6 | imm_5_3 | imm_2;
        }

        uint32_t get_imm_c_lwsp() {
            uint8_t imm_7_6 = (inst & M_IMM_3_2) << 4;
            uint8_t imm_5 = (inst & M_IMM_12) >> 7;
            uint8_t imm_4_2 = (inst & M_IMM_6_4) >> 2;
            return imm_7_6 | imm_5 | imm_4_2;
        }

        uint32_t get_imm_c_swsp() {
            uint8_t imm_7_6 = (inst & M_IMM_8_7) >> 1;
            uint8_t imm_5_2 = (inst & M_IMM_12_9) >> 7;
            return imm_7_6 | imm_5_2;
        }

        // arithmetic and logic operations
        uint32_t al_add(uint32_t a, uint32_t b) {
            return int32_t(a) + int32_t(b);
        };
        uint32_t al_sub(uint32_t a, uint32_t b) {
            return int32_t(a) - int32_t(b);
        };
        uint32_t al_sll(uint32_t a, uint32_t b) { return a << b; };
        uint32_t al_srl(uint32_t a, uint32_t b) { return a >> b; };
        uint32_t al_sra(uint32_t a, uint32_t b) {
            b &= 0x1f;
            return int32_t(a) >> b;
        };
        uint32_t al_slt(uint32_t a, uint32_t b) {
            return int32_t(a) < int32_t(b);
        };
        uint32_t al_sltu(uint32_t a, uint32_t b) { return a < b; };
        uint32_t al_xor(uint32_t a, uint32_t b) { return a ^ b; };
        uint32_t al_or(uint32_t a, uint32_t b) { return a | b; };
        uint32_t al_and(uint32_t a, uint32_t b) { return a & b; };

        // arithmetic and logic operations - M extension
        uint32_t al_mul(uint32_t a, uint32_t b) {
            return int32_t(a) * int32_t(b);
        };
        uint32_t al_mulh(uint32_t a, uint32_t b) {
            int64_t res = int64_t(int32_t(a)) * int64_t(int32_t(b));
            return res >> 32;
        };
        uint32_t al_mulhsu(uint32_t a, uint32_t b) {
            int64_t res = int64_t(int32_t(a)) * int64_t(b);
            return res >> 32;
        };
        uint32_t al_mulhu(uint32_t a, uint32_t b) {
            uint64_t res = uint64_t(a) * uint64_t(b);
            return res >> 32;
        };
        uint32_t al_div(uint32_t a, uint32_t b) {
            // division by zero
            if (b == 0) return -1;
            // overflow (most negative int divided by -1)
            if (a == 0x80000000 && b == 0xffffffff) return a;
            return int32_t(a) / int32_t(b);
        };
        uint32_t al_divu(uint32_t a, uint32_t b) {
            if (b == 0) return 0xffffffff;
            return a / b;
        };
        uint32_t al_rem(uint32_t a, uint32_t b) {
            if (b == 0) return a;
            if (a == 0x80000000 && b == 0xffffffff) return 0;
            return int32_t(a) % int32_t(b);
        };
        uint32_t al_remu(uint32_t a, uint32_t b) {
            if (b == 0) return a;
            return a % b;
        };

        // arithmetic and logic immediate operations
        uint32_t al_addi(uint32_t a, uint32_t b) { return al_add(a, b); };
        uint32_t al_slli(uint32_t a, uint32_t b) { return al_sll(a, b); };
        uint32_t al_srli(uint32_t a, uint32_t b) { return al_srl(a, b); };
        uint32_t al_srai(uint32_t a, uint32_t b) { return al_sra(a, b); };
        uint32_t al_slti(uint32_t a, uint32_t b) {
            if (inst == INST_HINT_LOG_START) {
                #ifdef ENABLE_DASM
                logging = true;
                #endif
                #ifdef ENABLE_PROF
                prof.active = true;
                #endif
                return 0;
            } else if (inst == INST_HINT_LOG_END) {
                #ifdef ENABLE_DASM
                logging = false;
                #endif
                #ifdef ENABLE_PROF
                prof.active = false;
                #endif
                return 0;
            }

            return al_slt(a, b);
        };
        uint32_t al_sltiu(uint32_t a, uint32_t b) { return al_sltu(a, b); };
        uint32_t al_xori(uint32_t a, uint32_t b) { return al_xor(a, b); };
        uint32_t al_ori(uint32_t a, uint32_t b) { return al_or(a, b); };
        uint32_t al_andi(uint32_t a, uint32_t b) { return al_and(a, b); };

        // load operations
        uint32_t load_lb(uint32_t addr) {
            PROF_DMEM(1)
            return TO_U32(TO_I8(mem->rd8(addr)));
        }
        uint32_t load_lh(uint32_t addr) {
            PROF_DMEM(2)
            return TO_U32(TO_I16(mem->rd16(addr)));
        }
        uint32_t load_lw(uint32_t addr) {
            PROF_DMEM(4)
            return mem->rd32(addr);
        }
        uint32_t load_lbu(uint32_t addr) {
            PROF_DMEM(1)
            return mem->rd8(addr);
        }
        uint32_t load_lhu(uint32_t addr) {
            PROF_DMEM(2)
            return mem->rd16(addr);
        }

        // store operations
        void store_sb(uint32_t addr, uint32_t data) {
            PROF_DMEM((1 | 0x8))
            mem->wr8(addr, data);
        }
        void store_sh(uint32_t addr, uint32_t data) {
            PROF_DMEM((2 | 0x8))
            mem->wr16(addr, data);
        }
        void store_sw(uint32_t addr, uint32_t data) {
            PROF_DMEM((4 | 0x8))
            mem->wr32(addr, data);
        }

        // branch operations
        bool branch_beq() { return rf[get_rs1()] == rf[get_rs2()]; }
        bool branch_bne() { return rf[get_rs1()] != rf[get_rs2()]; }
        bool branch_blt() {
            return int32_t(rf[get_rs1()]) < int32_t(rf[get_rs2()]);
        }
        bool branch_bge() {
            return int32_t(rf[get_rs1()]) >= int32_t(rf[get_rs2()]);
        }
        bool branch_bltu() {
            return (uint32_t)rf[get_rs1()] < (uint32_t)rf[get_rs2()];
        }
        bool branch_bgeu() {
            return (uint32_t)rf[get_rs1()] >= (uint32_t)rf[get_rs2()];
        }

        // csr operations
        void csr_access();
        void csr_rw(uint32_t init_val_rs1) { W_CSR(init_val_rs1); }
        void csr_rs(uint32_t init_val_rs1) {
            if (get_rs1() == 0) return;
            W_CSR(csr.at(get_csr_addr()).value | init_val_rs1);
        }
        void csr_rc(uint32_t init_val_rs1) {
            if (get_rs1() == 0) return;
            W_CSR(csr.at(get_csr_addr()).value & ~init_val_rs1);
        }
        void csr_rwi() { W_CSR(get_uimm_csr()); }
        void csr_rsi() {
            if (get_rs1() == 0) return;
            W_CSR(csr.at(get_csr_addr()).value | get_uimm_csr());
        }
        void csr_rci() {
            if (get_rs1() == 0) return;
            W_CSR(csr.at(get_csr_addr()).value & ~get_uimm_csr());
        }

        // C extension
        void c0();
        void c1();
        void c2();

        // C extension - arithmetic and logic operations
        void c_addi();
        void c_li();
        void c_lui();
        void c_nop();
        void c_addi16sp();
        void c_srli();
        void c_srai();
        void c_andi();
        void c_and();
        void c_or();
        void c_xor();
        void c_sub();
        void c_addi4spn();
        void c_slli();
        void c_mv();
        void c_add();
        // C extension - memory operations
        void c_lw();
        void c_lwsp();
        void c_sw();
        void c_swsp();
        // C extension - control transfer operations
        void c_beqz();
        void c_bnez();
        void c_j();
        void c_jal();
        void c_jr();
        void c_jalr();
        // C extension - system
        void c_ebreak();

    private:
        bool running;
        bool logging;
        std::array<int32_t, 32> rf;
        uint32_t pc;
        uint32_t next_pc;
        uint32_t inst;
        uint64_t inst_cnt;
        uint64_t inst_cnt_csr;
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        std::chrono::time_point<std::chrono::high_resolution_clock> run_time;
        // FIXME: mtime should be an actual MMIO put somewhere in the memory map
        uint64_t mtime;
        std::string log_name;
        #if defined(LOG_EXEC) or defined(LOG_EXEC_ALL)
        std::ofstream log_ofstream;
        #endif
        #if defined(LOG_EXEC_ALL)
        std::ostringstream mem_ostr;
        #endif
        #ifdef ENABLE_DASM
        dasm_str dasm;
        #endif
        memory *mem;
        static constexpr std::array<CSR_entry, 14> supported_csrs = {{
            {CSR_TOHOST, "tohost", perm_t::rw, 0u},
            {CSR_MSCRATCH, "mscratch", perm_t::rw, 0u},
            {CSR_MCYCLE, "mcycle", perm_t::rw, 0u},
            {CSR_MINSTRET, "minstret", perm_t::rw, 0u},
            {CSR_MCYCLEH, "mcycleh", perm_t::rw, 0u},
            {CSR_MINSTRETH, "minstreth", perm_t::rw, 0u},
            // read only CSRs
            {CSR_MISA, "misa", perm_t::warl_unimp, 0u},
            {CSR_MHARTID, "mhartid", perm_t::ro, 0u},
            // read only user CSRs
            {CSR_CYCLE, "cycle", perm_t::ro, 0u},
            {CSR_TIME, "time", perm_t::ro, 0u},
            {CSR_INSTRET, "instret", perm_t::ro, 0u},
            {CSR_CYCLEH, "cycleh", perm_t::ro, 0u},
            {CSR_TIMEH, "timeh", perm_t::ro, 0u},
            {CSR_INSTRETH, "instreth", perm_t::ro, 0u},
        }};
        #ifdef ENABLE_PROF
        profiler prof;
        #endif

        // csr map
        std::map<uint16_t, CSR> csr;

        // register names
        static constexpr std::array<std::array<const char*, 2>, 32>
        rf_names = {{
            {{"x0", "zero"}}, // hard-wired zero
            {{"x1", "ra"}},   // return address
            {{"x2", "sp"}},   // stack pointer
            {{"x3", "gp"}},   // global pointer
            {{"x4", "tp"}},   // thread pointer
            {{"x5", "t0"}},   // temporary/alternate link register
            {{"x6", "t1"}},   // temporary
            {{"x7", "t2"}},   // temporary
            {{"x8", "s0"}},   // saved register/frame pointer
            {{"x9", "s1"}},   // saved register
            {{"x10", "a0"}},  // function argument/return value
            {{"x11", "a1"}},  // function argument/return value
            {{"x12", "a2"}},  // function argument
            {{"x13", "a3"}},  // function argument
            {{"x14", "a4"}},  // function argument
            {{"x15", "a5"}},  // function argument
            {{"x16", "a6"}},  // function argument
            {{"x17", "a7"}},  // function argument
            {{"x18", "s2"}},  // saved register
            {{"x19", "s3"}},  // saved register
            {{"x20", "s4"}},  // saved register
            {{"x21", "s5"}},  // saved register
            {{"x22", "s6"}},  // saved register
            {{"x23", "s7"}},  // saved register
            {{"x24", "s8"}},  // saved register
            {{"x25", "s9"}},  // saved register
            {{"x26", "s10"}}, // saved register
            {{"x27", "s11"}}, // saved register
            {{"x28", "t3"}},  // temporary
            {{"x29", "t4"}},  // temporary
            {{"x30", "t5"}},  // temporary
            {{"x31", "t6"}},  // temporary
        }};
};

#pragma once

#include "defines.h"
#include "memory.h"
#include "profiler.h"

class core{
    public:
        core() = delete;
        core(uint32_t base_address, memory *mem, std::string log_name);
        void exec();
        void exec_inst();
        void dump();
        std::string dump_state();
        void finish(bool dump_regs);
        uint32_t get_pc() { return pc; }
        uint32_t get_inst() { return inst; }
        uint32_t get_reg(uint32_t reg) { return rf[reg]; }
        uint32_t get_inst_cnt() { return inst_cnt; }
        #ifdef ENABLE_DASM
        std::string get_inst_asm() { return dasm.asm_str; }
        #endif

    private:
        void write_rf(uint32_t reg, uint32_t data) { if(reg) rf[reg] = data; }
        void write_csr(uint16_t addr, uint32_t data) { csr.at(addr).value = data; }

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
        // void reset();

        // instruction parsing
        uint32_t get_opcode() { return (inst & M_OPC7); }
        //uint32_t get_funct7() { return (inst & M_FUNCT7) >> 25; }
        uint32_t get_funct7_b5() { return (inst & M_FUNCT7_B5) >> 30; }
        uint32_t get_funct7_b1() { return (inst & M_FUNCT7_B1) >> 25; }
        uint32_t get_funct3() { return (inst & M_FUNCT3) >> 12; }
        uint32_t get_rd() { return (inst & M_RD) >> 7; }
        uint32_t get_rs1() { return (inst & M_RS1) >> 15; }
        uint32_t get_rs2() { return (inst & M_RS2) >> 20; }
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

        // arithmetic and logic operations
        uint32_t al_add(uint32_t a, uint32_t b) {
            return int32_t(a) + int32_t(b);
        };
        uint32_t al_sub(uint32_t a, uint32_t b) {
            return int32_t(a) - int32_t(b);
        };
        uint32_t al_sll(uint32_t a, uint32_t b) {
            return a << b;
        };
        uint32_t al_srl(uint32_t a, uint32_t b) {
            return a >> b;
        };
        uint32_t al_sra(uint32_t a, uint32_t b) {
            b &= 0x1f;
            return int32_t(a) >> b;
        };
        uint32_t al_slt(uint32_t a, uint32_t b) {
            return int32_t(a) < int32_t(b);
        };
        uint32_t al_sltu(uint32_t a, uint32_t b) {
            return a < b;
        };
        uint32_t al_xor(uint32_t a, uint32_t b) {
            return a ^ b;
        };
        uint32_t al_or(uint32_t a, uint32_t b) {
            return a | b;
        };
        uint32_t al_and(uint32_t a, uint32_t b) {
            return a & b;
        };

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
        uint32_t al_addi(uint32_t a, uint32_t b) {
            return int32_t(a) + int32_t(b);
        };
        uint32_t al_slli(uint32_t a, uint32_t b) {
            return a << b;
        };
        uint32_t al_srli(uint32_t a, uint32_t b) {
            return a >> b;
        };
        uint32_t al_srai(uint32_t a, uint32_t b) {
            b &= 0x1f;
            return int32_t(a) >> b;
        };
        uint32_t al_slti(uint32_t a, uint32_t b) {
            return int32_t(a) < int32_t(b);
        };
        uint32_t al_sltiu(uint32_t a, uint32_t b) {
            return a < b;
        };
        uint32_t al_xori(uint32_t a, uint32_t b) {
            return a ^ b;
        };
        uint32_t al_ori(uint32_t a, uint32_t b) {
            return a | b;
        };
        uint32_t al_andi(uint32_t a, uint32_t b) {
            return a & b;
        };

        // load operations
        uint32_t load_lb(uint32_t address) {
            return static_cast<uint32_t>(
                static_cast<int8_t>(mem->rd8(address)));
        }
        uint32_t load_lh(uint32_t address) {
            return static_cast<uint32_t>(
                static_cast<int16_t>(mem->rd16(address)));
        }
        uint32_t load_lw(uint32_t address) {
            return mem->rd32(address);
        };
        uint32_t load_lbu(uint32_t address) {
            return mem->rd8(address);
        };
        uint32_t load_lhu(uint32_t address) {
            return mem->rd16(address);
        };

        // store operations
        void store_sb(uint32_t address, uint32_t data) {
            mem->wr8(address, data);
        }
        void store_sh(uint32_t address, uint32_t data) {
            mem->wr16(address, data);
        }
        void store_sw(uint32_t address, uint32_t data) {
            mem->wr32(address, data);
        }

        // branch operations
        bool branch_beq() {
            return rf[get_rs1()] == rf[get_rs2()];
        };
        bool branch_bne() {
            return rf[get_rs1()] != rf[get_rs2()];
        };
        bool branch_blt() {
            return int32_t(rf[get_rs1()]) < int32_t(rf[get_rs2()]);
        };
        bool branch_bge() {
            return int32_t(rf[get_rs1()]) >= int32_t(rf[get_rs2()]);
        };
        bool branch_bltu() {
            return (uint32_t)rf[get_rs1()] < (uint32_t)rf[get_rs2()];
        }
        bool branch_bgeu() {
            return (uint32_t)rf[get_rs1()] >= (uint32_t)rf[get_rs2()];
        }

        // csr operations
        void csr_access();
        void csr_rw(uint32_t init_val_rs1) {
            W_CSR(init_val_rs1);
        }
        void csr_rs(uint32_t init_val_rs1) {
            W_CSR(csr.at(get_csr_addr()).value | init_val_rs1);
        }
        void csr_rc(uint32_t init_val_rs1) {
            W_CSR(csr.at(get_csr_addr()).value & ~init_val_rs1);
        }
        void csr_rwi() {
            W_CSR(get_uimm_csr());
        }
        void csr_rsi() {
            W_CSR(csr.at(get_csr_addr()).value | get_uimm_csr());
        }
        void csr_rci() {
            W_CSR(csr.at(get_csr_addr()).value & ~get_uimm_csr());
        }

    private:
        bool running;
        std::array<int32_t, 32> rf;
        uint32_t pc;
        uint32_t next_pc;
        uint32_t inst;
        uint64_t inst_cnt;
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
        static constexpr std::array<CSR_entry, 2> supported_csrs = {{
            {0x51e, "tohost"},
            {0x340, "mscratch"}
        }};
        #ifdef ENABLE_PROF
        profiler prof;
        #endif

        // csr map
        std::unordered_map<uint16_t, CSR> csr;

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

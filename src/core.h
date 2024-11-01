#pragma once

#include "defines.h"
#include "memory.h"
#include "inst_parser.h"
#include "profiler.h"
#include "profiler_fusion.h"

class core{
    public:
        core() = delete;
        core(uint32_t base_addr, memory *mem, std::string log_name,
             logging_pc_t logging_pc);
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
            if (csr.at(addr).perm == csr_perm_t::ro)
                illegal("CSR write attempt to RO CSR", 4);
            if (csr.at(addr).perm == csr_perm_t::warl_unimp)
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
        void custom_ext();
        void unsupported(const std::string &msg);
        void illegal(const std::string &msg, uint32_t memw);
        // void reset();

        // instruction parsing
        inst_parser ip;
        void inst_fetch() {
            inst = mem->get_inst(pc);
            ip.inst = inst;
        }

        // arithmetic and logic operations
        uint32_t al_add(uint32_t a, uint32_t b) {
            return TO_I32(a) + TO_I32(b);
        };
        uint32_t al_sub(uint32_t a, uint32_t b) {
            return TO_I32(a) - TO_I32(b);
        };
        uint32_t al_sll(uint32_t a, uint32_t b) { return a << b; };
        uint32_t al_srl(uint32_t a, uint32_t b) { return a >> b; };
        uint32_t al_sra(uint32_t a, uint32_t b) {
            b &= 0x1f;
            return TO_I32(a) >> b;
        };
        uint32_t al_slt(uint32_t a, uint32_t b) {
            return TO_I32(a) < TO_I32(b);
        };
        uint32_t al_sltu(uint32_t a, uint32_t b) { return a < b; };
        uint32_t al_xor(uint32_t a, uint32_t b) { return a ^ b; };
        uint32_t al_or(uint32_t a, uint32_t b) { return a | b; };
        uint32_t al_and(uint32_t a, uint32_t b) { return a & b; };

        // arithmetic and logic operations - M extension
        uint32_t al_mul(uint32_t a, uint32_t b) {
            return TO_I32(a) * TO_I32(b);
        };
        uint32_t al_mulh(uint32_t a, uint32_t b) {
            int64_t res = TO_I64(TO_I32(a)) * TO_I64(TO_I32(b));
            return res >> 32;
        };
        uint32_t al_mulhsu(uint32_t a, uint32_t b) {
            int64_t res = TO_I64(TO_I32(a)) * TO_I64(b);
            return res >> 32;
        };
        uint32_t al_mulhu(uint32_t a, uint32_t b) {
            uint64_t res = TO_U64(a) * TO_U64(b);
            return res >> 32;
        };
        uint32_t al_div(uint32_t a, uint32_t b) {
            // division by zero
            if (b == 0) return -1;
            // overflow (most negative int divided by -1)
            if (a == 0x80000000 && b == 0xffffffff) return a;
            return TO_I32(a) / TO_I32(b);
        };
        uint32_t al_divu(uint32_t a, uint32_t b) {
            if (b == 0) return 0xffffffff;
            return a / b;
        };
        uint32_t al_rem(uint32_t a, uint32_t b) {
            if (b == 0) return a;
            if (a == 0x80000000 && b == 0xffffffff) return 0;
            return TO_I32(a) % TO_I32(b);
        };
        uint32_t al_remu(uint32_t a, uint32_t b) {
            if (b == 0) return a;
            return a % b;
        };

        // arithmetic and logic immediate operations
        uint32_t al_addi(uint32_t a, uint32_t b) { return al_add(a, b); };
        uint32_t al_slli(uint32_t a, uint32_t b) {
            #ifdef ENABLE_PROF
            prof_fusion.attack(
                {trigger::slli_lea, inst, mem->get_inst(pc + 4), false}
            );
            #endif
            return al_sll(a, b);
        };
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
        bool branch_beq() { return rf[ip.rs1()] == rf[ip.rs2()]; }
        bool branch_bne() { return rf[ip.rs1()] != rf[ip.rs2()]; }
        bool branch_blt() {
            return TO_I32(rf[ip.rs1()]) < TO_I32(rf[ip.rs2()]);
        }
        bool branch_bge() {
            return TO_I32(rf[ip.rs1()]) >= TO_I32(rf[ip.rs2()]);
        }
        bool branch_bltu() {
            return TO_U32(rf[ip.rs1()]) < TO_U32(rf[ip.rs2()]);
        }
        bool branch_bgeu() {
            return TO_U32(rf[ip.rs1()]) >= TO_U32(rf[ip.rs2()]);
        }

        // csr operations
        void csr_access();
        void csr_rw(uint32_t init_val_rs1) { W_CSR(init_val_rs1); }
        void csr_rs(uint32_t init_val_rs1) {
            if (ip.rs1() == 0) return;
            W_CSR(csr.at(ip.csr_addr()).value | init_val_rs1);
        }
        void csr_rc(uint32_t init_val_rs1) {
            if (ip.rs1() == 0) return;
            W_CSR(csr.at(ip.csr_addr()).value & ~init_val_rs1);
        }
        void csr_rwi() { W_CSR(ip.uimm_csr()); }
        void csr_rsi() {
            if (ip.rs1() == 0) return;
            W_CSR(csr.at(ip.csr_addr()).value | ip.uimm_csr());
        }
        void csr_rci() {
            if (ip.rs1() == 0) return;
            W_CSR(csr.at(ip.csr_addr()).value & ~ip.uimm_csr());
        }

        // custom extension
        uint32_t al_c_fma16(uint32_t a, uint32_t b);
        uint32_t al_c_fma8(uint32_t a, uint32_t b);
        uint32_t al_c_fma4(uint32_t a, uint32_t b);

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

        #ifdef ENABLE_HW_PROF
        void log_hw_stats();
        #endif

    private:
        bool running;
        bool logging;
        logging_pc_t logging_pc;
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
            {CSR_TOHOST, "tohost", csr_perm_t::rw, 0u},
            {CSR_MSCRATCH, "mscratch", csr_perm_t::rw, 0u},
            {CSR_MCYCLE, "mcycle", csr_perm_t::rw, 0u},
            {CSR_MINSTRET, "minstret", csr_perm_t::rw, 0u},
            {CSR_MCYCLEH, "mcycleh", csr_perm_t::rw, 0u},
            {CSR_MINSTRETH, "minstreth", csr_perm_t::rw, 0u},
            // read only CSRs
            {CSR_MISA, "misa", csr_perm_t::warl_unimp, 0u},
            {CSR_MHARTID, "mhartid", csr_perm_t::ro, 0u},
            // read only user CSRs
            {CSR_CYCLE, "cycle", csr_perm_t::ro, 0u},
            {CSR_TIME, "time", csr_perm_t::ro, 0u},
            {CSR_INSTRET, "instret", csr_perm_t::ro, 0u},
            {CSR_CYCLEH, "cycleh", csr_perm_t::ro, 0u},
            {CSR_TIMEH, "timeh", csr_perm_t::ro, 0u},
            {CSR_INSTRETH, "instreth", csr_perm_t::ro, 0u},
        }};
        #ifdef ENABLE_PROF
        profiler prof;
        profiler_fusion prof_fusion;
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

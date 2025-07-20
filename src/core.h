#pragma once

#include "defines.h"
#include "memory.h"
#include "inst_parser.h"
#include "trap.h"

#ifdef PROFILERS_EN
#include "profiler.h"
#include "profiler_perf.h"
#include "profiler_fusion.h"
#endif

#ifdef HW_MODELS_EN
#include "bp_if.h"
#endif

class core {
    public:
        core() = delete;
        core(memory* mem, cfg_t cfg, hw_cfg_t hw_cfg);
        void exec();
        void exec_inst();
        void dump();
        std::string print_state(bool dump_csr);
        void finish(bool dump_regs);
        #ifdef DPI
        uint32_t get_pc() { return pc; }
        uint32_t get_inst() { return inst; }
        uint32_t get_reg(uint32_t reg) { return rf[reg]; }
        uint32_t get_inst_cnt() { return inst_cnt; }
        void update_clk(uint64_t clk) { clk_src.update(clk); }
        #endif
        #ifdef PROFILERS_EN
        uint8_t inst_w = 8;
        std::string get_callstack_top_str() {
            return prof_perf.get_callstack_top_str();
        }
        #endif
        #ifdef DASM_EN
        std::string get_inst_asm() { return dasm.asm_str; }
        std::string get_rd_val_str() {
            std::ostringstream ostr;
            ostr << "x" << ip.rd() << ": " << FHEXZ(rf[ip.rd()], 8);
            return ostr.str();
        }
        void simd_ss_init(std::string a);
        void simd_ss_init(std::string a, std::string b);
        void simd_ss_init(std::string c, std::string a, std::string b);
        void simd_ss_append(int32_t a);
        void simd_ss_append(int32_t a, int32_t b);
        void simd_ss_append(int32_t c, int32_t a, int32_t b);
        void simd_ss_finish(std::string a);
        void simd_ss_finish(std::string a, std::string b, int32_t res);
        void simd_ss_finish(std::string c, std::string a, std::string b);
        #endif

    private:
        void write_rf(uint32_t reg, uint32_t data) { if (reg) rf[reg] = data; }
        void write_rf_pair(uint32_t reg, reg_pair rp) {
            if (reg == 31) tu.e_hardware_error("Write to x31");
            if (reg) {
                rf[reg] = rp.a;
                rf[reg + 1] = rp.b;
            }
        }
        void write_csr(uint16_t addr, uint32_t data) {
            if (csr.at(addr).perm == csr_perm_t::ro) {
                tu.e_illegal_inst("CSR write attempt to RO CSR", 4);
            }
            if (csr.at(addr).perm == csr_perm_t::warl_unimp) return;
            csr.at(addr).value = data;
            // FIXME: find a better way to handle RO bits in otherwise RW CSRs
            // MPP always in machine mode (0x3)
            if (addr == CSR_MSTATUS) csr.at(addr).value |= 0x1800;
        }
        void cntr_update();
        void prof_state(bool enable);

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
        // void reset();

        // instruction parsing
        inst_parser ip;
        void inst_fetch() {
            #ifdef HW_MODELS_EN
            // if previous inst was branch, use that instead of fetching
            // this prevents cache from logging the same access twice
            if (!last_inst_branch) {
                inst = mem->rd_inst(pc);
            } else {
                inst = inst_resolved;
                hwrs.ic_hm = next_ic_hm;
                next_ic_hm = hw_status_t::none;
            }
            last_inst_branch = false;
            #else
            inst = mem->rd_inst(pc);
            #endif
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

        // arithmetic and logic operations - Zbb extension partial
        uint32_t al_max(uint32_t a, uint32_t b) {
            return TO_I32(a) > TO_I32(b) ? a : b;
        };

        uint32_t al_maxu(uint32_t a, uint32_t b) {
            return a > b ? a : b;
        };

        uint32_t al_min(uint32_t a, uint32_t b) {
            return TO_I32(a) < TO_I32(b) ? a : b;
        };

        uint32_t al_minu(uint32_t a, uint32_t b) {
            return a < b ? a : b;
        };

        // arithmetic and logic immediate operations
        uint32_t al_addi(uint32_t a, uint32_t b) { return al_add(a, b); };
        uint32_t al_slli(uint32_t a, uint32_t b) {
            #ifdef PROFILERS_EN
            prof_fusion.attack(
                {trigger::slli_lea, inst, mem->just_inst(pc + 4), false}
            );
            #endif
            return al_sll(a, b);
        };
        uint32_t al_srli(uint32_t a, uint32_t b) { return al_srl(a, b); };
        uint32_t al_srai(uint32_t a, uint32_t b) { return al_sra(a, b); };
        uint32_t al_slti(uint32_t a, uint32_t b) {
            if (inst == INST_HINT_LOG_START) {
                prof_state(prof_pc.should_start());
                return 0;
            } else if (inst == INST_HINT_LOG_END) {
                prof_state(false);
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
            return TO_U32(TO_I8(mem->rd(addr, 1u)));
        }
        uint32_t load_lh(uint32_t addr) {
            PROF_DMEM(2)
            return TO_U32(TO_I16(mem->rd(addr, 2u)));
        }
        uint32_t load_lw(uint32_t addr) {
            PROF_DMEM(4)
            return mem->rd(addr, 4u);
        }
        uint32_t load_lbu(uint32_t addr) {
            PROF_DMEM(1)
            return mem->rd(addr, 1u);
        }
        uint32_t load_lhu(uint32_t addr) {
            PROF_DMEM(2)
            return mem->rd(addr, 2u);
        }

        // store operations
        void store_sb(uint32_t addr, uint32_t data) {
            PROF_DMEM((1 | 0x8))
            mem->wr(addr, data, 1u);
        }
        void store_sh(uint32_t addr, uint32_t data) {
            PROF_DMEM((2 | 0x8))
            mem->wr(addr, data, 2u);
        }
        void store_sw(uint32_t addr, uint32_t data) {
            PROF_DMEM((4 | 0x8))
            mem->wr(addr, data, 4u);
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

        // custom extension - arithmetic and logic operations
        uint32_t al_c_add16(uint32_t a, uint32_t b);
        uint32_t al_c_add8(uint32_t a, uint32_t b);
        uint32_t al_c_sub16(uint32_t a, uint32_t b);
        uint32_t al_c_sub8(uint32_t a, uint32_t b);
        reg_pair al_c_mul16(uint32_t a, uint32_t b);
        reg_pair al_c_mul16u(uint32_t a, uint32_t b);
        reg_pair al_c_mul8(uint32_t a, uint32_t b);
        reg_pair al_c_mul8u(uint32_t a, uint32_t b);
        uint32_t al_c_dot16(uint32_t a, uint32_t b);
        uint32_t al_c_dot8(uint32_t a, uint32_t b);
        uint32_t al_c_dot4(uint32_t a, uint32_t b);

        // custom extension - memory operations
        reg_pair mem_c_unpk16(uint32_t a);
        reg_pair mem_c_unpk16u(uint32_t a);
        reg_pair mem_c_unpk8(uint32_t a);
        reg_pair mem_c_unpk8u(uint32_t a);
        reg_pair mem_c_unpk4(uint32_t a);
        reg_pair mem_c_unpk4u(uint32_t a);
        reg_pair mem_c_unpk2(uint32_t a);
        reg_pair mem_c_unpk2u(uint32_t a);

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

        // interrupts
        // TODO

        #ifdef HW_MODELS_EN
        void log_hw_stats();
        #endif

    private:
        bool running;
        std::array<int32_t, 32> rf;
        memory* mem;
        uint32_t pc;
        uint32_t next_pc;
        uint32_t inst;
        uint64_t inst_cnt;
        uint64_t inst_cnt_csr;
        trap tu;
        uint8_t rf_names_idx;
        uint8_t rf_names_w;
        uint8_t csr_names_w;
        bool end_dump_state;
        std::string out_dir;
        prof_pc_t prof_pc;
        bool prof_act; // used for both profiling and hw models
        #ifdef PROFILERS_EN
        bool prof_trace;
        #endif
        #ifdef DASM_EN
        std::ofstream log_ofstream;
        bool csr_updated = false;
        dasm_str dasm;
        logging_flags_t logf;
        #endif
        cfg_t cfg;

        std::map<uint16_t, CSR> csr;
        static constexpr std::array<CSR_entry, 25> supported_csrs = {{
            {CSR_TOHOST, "tohost", csr_perm_t::rw, 0u},

            // Machine Information Registers
            {CSR_MVENDORID, "mvendorid", csr_perm_t::warl_unimp, 0u},
            {CSR_MARCHID, "marchid", csr_perm_t::warl_unimp, 0u},
            {CSR_MIMPID, "mimpid", csr_perm_t::warl_unimp, 0u},
            {CSR_MHARTID, "mhartid", csr_perm_t::ro, 0u},
            {CSR_MCONFIGPTR, "mconfigptr", csr_perm_t::ro, 0u},

            // Machine Trap Setup
            {CSR_MSTATUS, "mstatus", csr_perm_t::rw, 0x1800}, // mpp = 3
            {CSR_MISA, "misa", csr_perm_t::warl_unimp, 0u},
            {CSR_MIE, "mie", csr_perm_t::warl, 0u},
            {CSR_MTVEC, "mtvec", csr_perm_t::rw, 0u},

            // Machine Trap Handling
            {CSR_MSCRATCH, "mscratch", csr_perm_t::rw, 0u},
            {CSR_MEPC, "mepc", csr_perm_t::rw, 0u},
            {CSR_MCAUSE, "mcause", csr_perm_t::rw, 0u},
            {CSR_MTVAL, "mtval", csr_perm_t::rw, 0u},
            {CSR_MIP, "mip", csr_perm_t::ro, 0u},

            // Machine Counter/Timers
            {CSR_MCYCLE, "mcycle", csr_perm_t::rw, 0u},
            {CSR_MINSTRET, "minstret", csr_perm_t::rw, 0u},
            {CSR_MCYCLEH, "mcycleh", csr_perm_t::rw, 0u},
            {CSR_MINSTRETH, "minstreth", csr_perm_t::rw, 0u},

            // Unprivileged Counter/Timers
            {CSR_CYCLE, "cycle", csr_perm_t::ro, 0u},
            {CSR_TIME, "time", csr_perm_t::ro, 0u},
            {CSR_INSTRET, "instret", csr_perm_t::ro, 0u},
            {CSR_CYCLEH, "cycleh", csr_perm_t::ro, 0u},
            {CSR_TIMEH, "timeh", csr_perm_t::ro, 0u},
            {CSR_INSTRETH, "instreth", csr_perm_t::ro, 0u},
        }};

        #ifdef PROFILERS_EN
        profiler prof;
        profiler_perf prof_perf;
        profiler_fusion prof_fusion;
        #ifdef DPI
        clock_source_t clk_src;
        #endif
        #endif

        #ifdef HW_MODELS_EN
        bp_if bp;
        uint32_t inst_speculative;
        uint32_t inst_resolved;
        bool last_inst_branch;
        bool no_bp;
        hw_status_t next_ic_hm;
        hw_running_stats_t hwrs;
        #endif

        // register names
        static constexpr std::array<std::array<std::string_view, 2>, 32>
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

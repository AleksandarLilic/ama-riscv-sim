#pragma once

#include "defines.h"
#include "csrs.h"
#include "hw_model_types.h"
#include "memory.h"
#include "inst_parser.h"
#include "trap.h"

#ifdef PROFILERS_EN
#include "profiler.h"
#include "profiler_perf.h"
#include "profiler_fusion.h"
#include "profiler_rf.h"
#endif

#ifdef HW_MODELS_EN
#include "bp_if.h"
#include "divider.h"
#endif

#ifdef DPI
#include "cosim.h"
#endif

class core {
    public:
        core() = delete;
        core(memory* mem, cfg_t cfg, hw_cfg_t hw_cfg);
        uint64_t run();
        void single_step();
        bool check_interrupts(bool defer_trap);
        void fetch();
        void exec();
        #ifdef DASM_EN
        void log_inst(bool trapped, bool log_symbol);
        #endif
        void dump();
        std::string print_state(bool dump_csr);
        void finish(bool dump_regs);

        #ifdef DPI
        uint32_t get_pc() { return pc; }
        uint32_t get_inst() { return inst; }
        uint32_t get_csr(uint16_t addr) { return csr.at(addr).value; }
        uint32_t get_reg(uint32_t reg) { return rf[reg]; }
        uint64_t get_inst_cnt() { return sim_cnt.inst; }
        void update_clk(uint64_t clk) { clk_src.update(clk); }
        void save_trace_entry(trace_entry te);
        void set_perf_event_flag(perf_event_t perf_event, bool set) {
            prof_perf.set_perf_event_flag(perf_event, set);
        }
        void force_irq(bool mtip, bool meip) {
            if (mtip) csr.at(csr_map::addr::mip).value |= csr_map::mip::mtip;
            if (meip) csr.at(csr_map::addr::mip).value |= csr_map::mip::meip;
        }
        #endif

        #if defined(PROFILERS_EN) || defined(DASM_EN)
        uint8_t inst_w = 8;
        #endif

        #ifdef PROFILERS_EN
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
        #ifdef SIMD_EN
        void simd_ss_clear();
        void simd_ss_init_c();
        void simd_ss_init_a();
        void simd_ss_init_ab();
        void simd_ss_init_ca();
        void simd_ss_init_cab();
        void simd_ss_append_a(int32_t a);
        void simd_ss_append_c(int32_t c);
        void simd_ss_append_ab(int32_t a, int32_t b);
        void simd_ss_append_ab_u(uint32_t a, uint32_t b);
        void simd_ss_append_cab(int32_t c, int32_t a, int32_t b);
        void simd_ss_append_imm(int32_t c, int32_t a, size_t w);
        void simd_ss_finish_ca();
        void simd_ss_finish_cas(int32_t shamt);
        void simd_ss_finish_cab();
        void simd_ss_finish_dot(int32_t res, int32_t c_in);
        void simd_ss_finish_dup(int32_t a_scalar);
        void simd_ss_finish_vins(int32_t a, int32_t idx);
        void simd_ss_finish_vext(int32_t c_scalar, int32_t idx);
        #endif // SIMD_EN
        #endif // DASM_EN

    private:
        void write_rf(uint32_t reg, uint32_t data) {
            if (reg) {
                rf[reg] = data;
                PROF_SPARSITY_ANY(data)
            }
        }

        void write_rf_pair(uint32_t reg, reg_pair rp) {
            if (reg == 31) tu.e_hardware_error("RDP Write to x31");
            if (reg) {
                rf[reg] = rp.a;
                rf[reg + 1] = rp.b;
                PROF_SPARSITY_ANY(rp.a)
                PROF_SPARSITY_ANY(rp.b)
            }
        }

        void write_csr(uint16_t addr, uint32_t data) {
            csr_def::CSR& c = csr.at(addr);
            if (c.perm == csr_def::perm_t::ro) {
                tu.e_illegal_inst("CSR write attempt to RO CSR", 4);
                return;
            }
            if (c.perm == csr_def::perm_t::war0) return;
            // only writable bits change, hardwired bits keep their reset value
            c.value = (c.value & ~c.wmask) | (data & c.wmask);
        }

        void csr_wide_assign(uint16_t addr, uint64_t val) {
            csr.at(addr).value = TO_U32(val);
            csr.at(addr + csr_def::low_to_high_off).value = TO_U32(val >> 32);
        }

        void csr_wide_add(uint16_t addr, uint64_t delta) {
            const uint16_t high_addr = (addr + csr_def::low_to_high_off);
            const uint64_t value = (
                (TO_U64(csr.at(high_addr).value) << 32) | csr.at(addr).value
            );
            csr_wide_assign(addr, value + delta);
        }

        void prof_state(bool enable);

        #ifndef DPI
        void save_trace_entry();
        #endif

        // instruction parsing
        inst_parser ip;

        // instruction decoders
        void d_alu_reg();
        void d_alu_imm();
        void d_load();
        void d_store();
        void d_branch();
        void d_jalr();
        void d_jal();
        void d_lui();
        void d_auipc();
        void d_system();
        void d_misc_mem();
        #ifdef SIMD_EN
        void d_custom_ext();
        #endif
        void d_csr_access();

        // arithmetic and logic operations
        uint32_t alu_add(uint32_t a, uint32_t b);
        uint32_t alu_sub(uint32_t a, uint32_t b);
        uint32_t alu_sll(uint32_t a, uint32_t b);
        uint32_t alu_srl(uint32_t a, uint32_t b);
        uint32_t alu_sra(uint32_t a, uint32_t b);
        uint32_t alu_slt(uint32_t a, uint32_t b);
        uint32_t alu_sltu(uint32_t a, uint32_t b);
        uint32_t alu_xor(uint32_t a, uint32_t b);
        uint32_t alu_or(uint32_t a, uint32_t b);
        uint32_t alu_and(uint32_t a, uint32_t b);
        // upper
        void lui();
        void auipc();
        // arithmetic and logic operations - M extension
        uint32_t alu_mul(uint32_t a, uint32_t b);
        uint32_t alu_mulh(uint32_t a, uint32_t b);
        uint32_t alu_mulhsu(uint32_t a, uint32_t b);
        uint32_t alu_mulhu(uint32_t a, uint32_t b);
        uint32_t alu_div(uint32_t a, uint32_t b);
        uint32_t alu_divu(uint32_t a, uint32_t b);
        uint32_t alu_rem(uint32_t a, uint32_t b);
        uint32_t alu_remu(uint32_t a, uint32_t b);
        // arithmetic and logic operations - Zbb extension partial
        uint32_t alu_max(uint32_t a, uint32_t b);
        uint32_t alu_maxu(uint32_t a, uint32_t b);
        uint32_t alu_min(uint32_t a, uint32_t b);
        uint32_t alu_minu(uint32_t a, uint32_t b);
        // arithmetic and logic immediate operations
        uint32_t alu_addi(uint32_t a, uint32_t b);
        uint32_t alu_slli(uint32_t a, uint32_t b);
        uint32_t alu_srli(uint32_t a, uint32_t b);
        uint32_t alu_srai(uint32_t a, uint32_t b);
        uint32_t alu_slti(uint32_t a, uint32_t b);
        uint32_t alu_sltiu(uint32_t a, uint32_t b);
        uint32_t alu_xori(uint32_t a, uint32_t b);
        uint32_t alu_ori(uint32_t a, uint32_t b);
        uint32_t alu_andi(uint32_t a, uint32_t b);
        // load operations
        uint32_t load_lb(uint32_t addr);
        uint32_t load_lh(uint32_t addr);
        uint32_t load_lw(uint32_t addr);
        uint32_t load_lbu(uint32_t addr);
        uint32_t load_lhu(uint32_t addr);
        // store operations
        void store_sb(uint32_t addr, uint32_t data);
        void store_sh(uint32_t addr, uint32_t data);
        void store_sw(uint32_t addr, uint32_t data);
        // branch operations
        bool branch_beq();
        bool branch_bne();
        bool branch_blt();
        bool branch_bge();
        bool branch_bltu();
        bool branch_bgeu();
        // jump
        void jalr();
        void jal();

        // zicsr
        void csr_rw(uint32_t init_val_rs1);
        void csr_rs(uint32_t init_val_rs1);
        void csr_rc(uint32_t init_val_rs1);
        void csr_rwi();
        void csr_rsi();
        void csr_rci();
        void csr_cnt_update(uint16_t addr);

        // trap
        void trap_state_update(uint32_t cause, uint32_t tval);
        void mret_state_update();
        static void trap_state_update_cb( // callback for trap handler
            void* ctx, uint32_t cause, uint32_t tval);

        #ifdef SIMD_EN
        // custom extension - arithmetic add & sub
        uint32_t alu_c_add16(uint32_t a, uint32_t b);
        uint32_t alu_c_add8(uint32_t a, uint32_t b);
        uint32_t alu_c_sub16(uint32_t a, uint32_t b);
        uint32_t alu_c_sub8(uint32_t a, uint32_t b);
        uint32_t alu_c_qadd16(uint32_t a, uint32_t b);
        uint32_t alu_c_qadd16u(uint32_t a, uint32_t b);
        uint32_t alu_c_qadd8(uint32_t a, uint32_t b);
        uint32_t alu_c_qadd8u(uint32_t a, uint32_t b);
        uint32_t alu_c_qsub16(uint32_t a, uint32_t b);
        uint32_t alu_c_qsub16u(uint32_t a, uint32_t b);
        uint32_t alu_c_qsub8(uint32_t a, uint32_t b);
        uint32_t alu_c_qsub8u(uint32_t a, uint32_t b);
        // custom extension - arithmetic mul
        uint32_t alu_c_mul16(uint32_t a, uint32_t b);
        uint32_t alu_c_mul8(uint32_t a, uint32_t b);
        uint32_t alu_c_mulh16(uint32_t a, uint32_t b);
        uint32_t alu_c_mulh16u(uint32_t a, uint32_t b);
        uint32_t alu_c_mulh8(uint32_t a, uint32_t b);
        uint32_t alu_c_mulh8u(uint32_t a, uint32_t b);
        // custom extension - arithmetic wmul
        reg_pair alu_c_wmul16(uint32_t a, uint32_t b);
        reg_pair alu_c_wmul16u(uint32_t a, uint32_t b);
        reg_pair alu_c_wmul8(uint32_t a, uint32_t b);
        reg_pair alu_c_wmul8u(uint32_t a, uint32_t b);
        // custom extension - arithmetic dot
        uint32_t alu_c_dot16(uint32_t a, uint32_t b, uint32_t c);
        uint32_t alu_c_dot16u(uint32_t a, uint32_t b, uint32_t c);
        uint32_t alu_c_dot8(uint32_t a, uint32_t b, uint32_t c);
        uint32_t alu_c_dot8u(uint32_t a, uint32_t b, uint32_t c);
        uint32_t alu_c_dot4(uint32_t a, uint32_t b, uint32_t c);
        uint32_t alu_c_dot4u(uint32_t a, uint32_t b, uint32_t c);
        uint32_t alu_c_dot2(uint32_t a, uint32_t b, uint32_t c);
        uint32_t alu_c_dot2u(uint32_t a, uint32_t b, uint32_t c);
        // custom extension - min & max
        uint32_t alu_c_min16(uint32_t a, uint32_t b);
        uint32_t alu_c_min16u(uint32_t a, uint32_t b);
        uint32_t alu_c_min8(uint32_t a, uint32_t b);
        uint32_t alu_c_min8u(uint32_t a, uint32_t b);
        uint32_t alu_c_max16(uint32_t a, uint32_t b);
        uint32_t alu_c_max16u(uint32_t a, uint32_t b);
        uint32_t alu_c_max8(uint32_t a, uint32_t b);
        uint32_t alu_c_max8u(uint32_t a, uint32_t b);
        // custom extension - shift
        uint32_t alu_c_slli16(uint32_t a, uint32_t shamt);
        uint32_t alu_c_slli8(uint32_t a, uint32_t shamt);
        uint32_t alu_c_srli16(uint32_t a, uint32_t shamt);
        uint32_t alu_c_srli8(uint32_t a, uint32_t shamt);
        uint32_t alu_c_srai16(uint32_t a, uint32_t shamt);
        uint32_t alu_c_srai8(uint32_t a, uint32_t shamt);

        // custom extension - data formatting - widen
        reg_pair data_fmt_c_widen16(uint32_t a, uint32_t shamt);
        reg_pair data_fmt_c_widen16u(uint32_t a, uint32_t shamt);
        reg_pair data_fmt_c_widen8(uint32_t a, uint32_t shamt);
        reg_pair data_fmt_c_widen8u(uint32_t a, uint32_t shamt);
        reg_pair data_fmt_c_widen4(uint32_t a, uint32_t shamt);
        reg_pair data_fmt_c_widen4u(uint32_t a, uint32_t shamt);
        reg_pair data_fmt_c_widen2(uint32_t a, uint32_t shamt);
        reg_pair data_fmt_c_widen2u(uint32_t a, uint32_t shamt);
        // custom extension - data formatting - narrow
        uint32_t data_fmt_c_narrow32(uint32_t a, uint32_t b);
        uint32_t data_fmt_c_narrow16(uint32_t a, uint32_t b);
        uint32_t data_fmt_c_narrow8(uint32_t a, uint32_t b);
        uint32_t data_fmt_c_narrow4(uint32_t a, uint32_t b);
        // custom extension - data formatting - narrow saturating
        uint32_t data_fmt_c_qnarrow32(uint32_t a, uint32_t b);
        uint32_t data_fmt_c_qnarrow32u(uint32_t a, uint32_t b);
        uint32_t data_fmt_c_qnarrow16(uint32_t a, uint32_t b);
        uint32_t data_fmt_c_qnarrow16u(uint32_t a, uint32_t b);
        uint32_t data_fmt_c_qnarrow8(uint32_t a, uint32_t b);
        uint32_t data_fmt_c_qnarrow8u(uint32_t a, uint32_t b);
        uint32_t data_fmt_c_qnarrow4(uint32_t a, uint32_t b);
        uint32_t data_fmt_c_qnarrow4u(uint32_t a, uint32_t b);
        // custom extension - data formatting - swap anti-diagonal
        reg_pair data_fmt_c_txp16(uint32_t a, uint32_t b);
        reg_pair data_fmt_c_txp8(uint32_t a, uint32_t b);
        reg_pair data_fmt_c_txp4(uint32_t a, uint32_t b);
        reg_pair data_fmt_c_txp2(uint32_t a, uint32_t b);
        // custom extension - scalar-vector dup
        uint32_t data_fmt_c_dup16(uint32_t rs1);
        uint32_t data_fmt_c_dup8(uint32_t rs1);
        uint32_t data_fmt_c_dup4(uint32_t rs1);
        uint32_t data_fmt_c_dup2(uint32_t rs1);
        // custom extension - scalar-vector vins (insert scalar into lane)
        uint32_t data_fmt_c_vins16(uint32_t rd_val, uint32_t rs1, uint8_t idx);
        uint32_t data_fmt_c_vins8(uint32_t rd_val, uint32_t rs1, uint8_t idx);
        uint32_t data_fmt_c_vins4(uint32_t rd_val, uint32_t rs1, uint8_t idx);
        uint32_t data_fmt_c_vins2(uint32_t rd_val, uint32_t rs1, uint8_t idx);
        // custom extension - scalar-vector vext (extract lane to scalar)
        uint32_t data_fmt_c_vext16(uint32_t rs1, uint8_t idx);
        uint32_t data_fmt_c_vext16u(uint32_t rs1, uint8_t idx);
        uint32_t data_fmt_c_vext8(uint32_t rs1, uint8_t idx);
        uint32_t data_fmt_c_vext8u(uint32_t rs1, uint8_t idx);
        uint32_t data_fmt_c_vext4(uint32_t rs1, uint8_t idx);
        uint32_t data_fmt_c_vext4u(uint32_t rs1, uint8_t idx);
        uint32_t data_fmt_c_vext2(uint32_t rs1, uint8_t idx);
        uint32_t data_fmt_c_vext2u(uint32_t rs1, uint8_t idx);
        #endif // SIMD_EN

        #ifdef RV32C_EN
        // C extension
        void d_compressed_0();
        void d_compressed_1();
        void d_compressed_2();

        // C extension
        // arithmetic and logic operations
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
        // memory operations
        void c_lw();
        void c_lwsp();
        void c_sw();
        void c_swsp();
        // control transfer operations
        void c_beqz();
        void c_bnez();
        void c_j();
        void c_jal();
        void c_jr();
        void c_jalr();
        // system
        void c_ebreak();
        #endif // RV32C_EN

        #ifdef SIMD_EN
        // templates
        // arith
        template <size_t vbits, bool vsigned, alu_add_sub_op_t op, bool sat>
            uint32_t alu_c_add_sub_op(uint32_t a, uint32_t b);
        template <size_t vbits, bool vsigned>
            reg_pair alu_c_wmul_op(uint32_t a, uint32_t b);
        template <size_t vbits, bool vsigned, bool upper>
            uint32_t alu_c_mul_op(uint32_t a, uint32_t b);
        template <size_t vbits, bool vsigned>
            uint32_t alu_c_dot_op(uint32_t a, uint32_t b, uint32_t c);
        template <size_t vbits, bool vsigned, alu_min_max_op_t op>
            uint32_t alu_c_min_max_op(uint32_t a, uint32_t b);
        template <size_t vbits, bool arith, alu_shift_op_t op>
            uint32_t alu_c_shift_op(uint32_t a, uint32_t shamt);
        // data fmt
        template <size_t vbits, bool vsigned>
            reg_pair data_fmt_c_widen_t(uint32_t a, uint32_t shamt);
        template <size_t vbits, bool vsat, bool vsigned>
            uint32_t data_fmt_c_narrow_t(uint32_t a, uint32_t b);
        template <size_t vbits>
            reg_pair data_fmt_c_txp_t(uint32_t a, uint32_t b);
        template <size_t vbits>
            uint32_t data_fmt_c_dup_t(uint32_t rs1);
        template <size_t vbits>
            uint32_t data_fmt_c_vins_t(uint32_t rd, uint32_t rs1, uint8_t idx);
        template <size_t vbits, bool vsigned>
            uint32_t data_fmt_c_vext_t(uint32_t rs1, uint8_t idx);
        #endif // SIMD_EN

        #ifdef HW_MODELS_EN
        void log_hw_stats();
        #endif

    private:
        cfg_t cfg;

        // internal state
        bool running = false;
        struct wait_for_interrupt_t { bool active = false, pend = false; };
        wait_for_interrupt_t wfi;
        std::array<int32_t, 32> rf;
        memory* mem;
        uint32_t pc;
        uint32_t next_pc = 0;
        uint32_t inst = 0;
        std::map<uint16_t, csr_def::CSR> csr;

        // other state
        struct sim_cnt_t {uint64_t inst = 0, step = 0; };
        sim_cnt_t sim_cnt = {0, 0};
        sim_cnt_t csr_cnt;
        trap tu;
        uint8_t rf_names_idx;
        uint8_t rf_names_w;
        uint8_t csr_names_w;
        std::string out_dir;

        #ifdef PROFILERS_EN
        prof_pc_t prof_pc;
        bool prof_active = false;
        bool prof_trace;
        profiler prof;
        profiler_perf prof_perf;
        profiler_fusion prof_fusion;
        profiler_rf prof_rf;
        bool branch_taken;
        #ifdef DPI
        clock_source_t clk_src;
        #endif
        #endif

        #ifdef DPI
        // CSRs that increment via core logic (time, cycles, uarch counters);
        // these can't be emulated in cosim's timed environment, so the ISA sim
        // trusts the RTL on them (get_rtl_val in the testbench)
        // must be a subset of supported_csrs (below)
        static constexpr uint16_t csr_rtl_trusted[] = {
            csr_map::addr::time, csr_map::addr::timeh,
            csr_map::addr::cycle, csr_map::addr::cycleh,
            csr_map::addr::instret, csr_map::addr::instreth,
            csr_map::addr::mcycle, csr_map::addr::mcycleh,
            csr_map::addr::minstret, csr_map::addr::minstreth,
            csr_map::addr::mhpmcounter3, csr_map::addr::mhpmcounter4,
            csr_map::addr::mhpmcounter5, csr_map::addr::mhpmcounter6,
            csr_map::addr::mhpmcounter7, csr_map::addr::mhpmcounter8,
            csr_map::addr::mhpmcounter3h, csr_map::addr::mhpmcounter4h,
            csr_map::addr::mhpmcounter5h, csr_map::addr::mhpmcounter6h,
            csr_map::addr::mhpmcounter7h, csr_map::addr::mhpmcounter8h,
        };

        static constexpr bool is_rtl_trusted(uint16_t addr) {
            for (uint16_t a : csr_rtl_trusted) if (a == addr) return true;
            return false;
        }

        static constexpr bool mmio_rtl_trusted(uint32_t addr) {
            return (
                mem_map::addr_is_clint_mtime(addr) ||
                mem_map::addr_is_uart(addr)
            );
        }

        // every rtl-trusted CSR must be a supported CSR
        // asserted at compile time in the constructor
        static constexpr bool csr_rtl_trusted_all_supported() {
            for (uint16_t a : csr_rtl_trusted) {
                bool found = false;
                for (const auto& e : csr_def::supported_csrs)
                    if (e.addr == a) found = true;
                if (!found) return false;
            }
            return true;
        }
        #endif

        #ifdef HW_MODELS_EN
        std::string bp_name;
        bp_if bp;
        divider div;
        uint32_t inst_speculative;
        uint32_t inst_resolved;
        bool last_inst_branch;
        bool no_bp;
        hw_status_t next_ic_hm;
        hw_running_stats_t hwrs;
        #endif

        #ifdef DASM_EN
        std::ofstream log_ofstream;
        bool dasm_update_csr = false;
        dasm_str dasm;
        hwmi_str hwmi;
        logging_flags_t logf;
        #endif

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

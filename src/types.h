#pragma once

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <iomanip>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cmath>
#include <bitset>

constexpr int const_log2(int n, int p = 0) {
    return (n <= 1) ? p : const_log2(n >> 1, p + 1);
}

// Memory
namespace mem_map {
    constexpr uint32_t base_addr = 0x8000'0000;
    constexpr uint32_t mem_size = 131072; // 0x2'0000
    constexpr uint32_t addr_bits = const_log2(mem_size); // 17 bits
    constexpr uint32_t addr_mask = (mem_size - 1);
    constexpr uint32_t uart0_addr = 0x1001'3000;
    constexpr uint32_t uart0_rx_data_addr = (uart0_addr + 0x04);
    constexpr uint32_t uart0_tx_data_addr = (uart0_addr + 0x08);
    constexpr uint32_t uart_size = 12; // 3 32-bit registers per UART
    constexpr uint32_t clint_addr = 0x0200'0000;
    constexpr uint32_t clint_size = 32; // reserved for 4 64-bit registers
    constexpr uint32_t clint_mtime_addr = (clint_addr + 0x10); // 64-bit reg
    constexpr uint32_t clint_mtime_size = 8;

    // address-region predicates, half-open range [base, base + size)
    constexpr bool in_region(uint32_t addr, uint32_t base, uint32_t size) {
        return (addr >= base) && (addr < (base + size));
    }
    constexpr bool addr_is_main(uint32_t addr) {
        return in_region(addr, base_addr, mem_size);
    }
    constexpr bool addr_is_uart(uint32_t addr) {
        return in_region(addr, uart0_addr, uart_size);
    }
    constexpr bool addr_is_clint(uint32_t addr) {
        return in_region(addr, clint_addr, clint_size);
    }
    constexpr bool addr_is_clint_mtime(uint32_t addr) {
        return in_region(addr, clint_mtime_addr, clint_mtime_size);
    }
}

constexpr uint32_t mem_addr_bitwidth = 8; // digits in hex printout

using address_t = uint32_t;

struct norm_address_t {
    uint32_t v;
    explicit constexpr norm_address_t(uint32_t v) : v(v) {}
};

constexpr norm_address_t to_norm(address_t a) {
    return norm_address_t{a & mem_map::addr_mask};
}
constexpr address_t to_full(norm_address_t a) {
    return (a.v | mem_map::base_addr);
}

// CSR map
namespace csrm {
    // CSR numbers (12-bit). Addresses live in their own sub-namespace so a
    // register name (e.g. mie/mip) can be both an address and a bit-field group.
    namespace addr {
        constexpr uint16_t tohost = 0x51E;
        // Machine Information Registers
        constexpr uint16_t mvendorid = 0xF11; // MRO
        constexpr uint16_t marchid = 0xF12; // MRO
        constexpr uint16_t mimpid = 0xF13; // MRO
        constexpr uint16_t mhartid = 0xF14; // MRO
        constexpr uint16_t mconfigptr = 0xF15; // MRO
        // Machine Trap Setup
        constexpr uint16_t mstatus = 0x300; // MRW
        constexpr uint16_t misa = 0x301; // MRW
        constexpr uint16_t mie = 0x304; // MWARL
        constexpr uint16_t mtvec = 0x305; // MRW
        // Machine Trap Handling
        constexpr uint16_t mscratch = 0x340; // MRW
        constexpr uint16_t mepc = 0x341; // MRW
        constexpr uint16_t mcause = 0x342; // MRW
        constexpr uint16_t mtval = 0x343; // MRW
        constexpr uint16_t mip = 0x344; // MRO
        // Machine Counter/Timers
        constexpr uint16_t mcycle = 0xB00; // MRW
        constexpr uint16_t minstret = 0xB02; // MRW
        constexpr uint16_t mcycleh = 0xB80; // MRW
        constexpr uint16_t minstreth = 0xB82; // MRW
        // Machine Hardware Performance Monitor (MHPM) counters & events
        constexpr uint16_t mhpmcounter3 = 0xB03; // MRW
        constexpr uint16_t mhpmcounter4 = 0xB04; // MRW
        constexpr uint16_t mhpmcounter5 = 0xB05; // MRW
        constexpr uint16_t mhpmcounter6 = 0xB06; // MRW
        constexpr uint16_t mhpmcounter7 = 0xB07; // MRW
        constexpr uint16_t mhpmcounter8 = 0xB08; // MRW
        constexpr uint16_t mhpmcounter3h = 0xB83; // MRW
        constexpr uint16_t mhpmcounter4h = 0xB84; // MRW
        constexpr uint16_t mhpmcounter5h = 0xB85; // MRW
        constexpr uint16_t mhpmcounter6h = 0xB86; // MRW
        constexpr uint16_t mhpmcounter7h = 0xB87; // MRW
        constexpr uint16_t mhpmcounter8h = 0xB88; // MRW
        constexpr uint16_t mhpmevent3 = 0x323; // MRW
        constexpr uint16_t mhpmevent4 = 0x324; // MRW
        constexpr uint16_t mhpmevent5 = 0x325; // MRW
        constexpr uint16_t mhpmevent6 = 0x326; // MRW
        constexpr uint16_t mhpmevent7 = 0x327; // MRW
        constexpr uint16_t mhpmevent8 = 0x328; // MRW
        // Unprivileged Counter/Timers
        constexpr uint16_t cycle = 0xC00; // URO
        constexpr uint16_t time = 0xC01; // URO
        constexpr uint16_t instret = 0xC02; // URO
        constexpr uint16_t cycleh = 0xC80; // URO
        constexpr uint16_t timeh = 0xC81; // URO
        constexpr uint16_t instreth = 0xC82; // URO
    }

    enum class perm_t {
        ro = 0b00, // read-only
        rw = 0b01, // read-write
        warl = 0b10, // write-any-read-legal
        warl_unimp = 0b11, // warl unimplemented -> always returns 0
    };

    constexpr uint32_t tohost_early_exit = 0xF000'0000;
    constexpr uint16_t low_to_high_off = 0x80; // low counter CSR -> its *h pair
    constexpr uint32_t mhpmcounters_num = 6;
    constexpr uint32_t mstatus_v = 0x1800; // mpp = 3
    constexpr uint32_t misa_v = (
        (1u << 30) | // MXL = 1 (RV32)
        (1u << 8)  | // I
        (1u << 12) | // M
        (1u << 23)   // X (non-standard extensions present)
    );

    // MSTATUS bits
    namespace mstatus {
        constexpr uint32_t mie = 0x8;
        constexpr uint32_t mpie = 0x80;
    }

    // MIP bits - machine interrupt pending
    namespace mip {
        constexpr uint32_t msip = (1u << 3); // software
        constexpr uint32_t mtip = (1u << 7); // timer
        constexpr uint32_t meip = (1u << 11); // external
        constexpr uint32_t lcofip = (1u << 13); // local counter overflow
    }

    // MIE bits - machine interrupt enable (same positions as MIP)
    namespace mie {
        constexpr uint32_t msie = mip::msip;
        constexpr uint32_t mtie = mip::mtip;
        constexpr uint32_t meie = mip::meip;
        constexpr uint32_t lcofie = mip::lcofip;
    }

    // MCAUSE codes
    namespace mcause {
        // exception codes
        constexpr uint32_t inst_addr_misaligned = 0x0;
        constexpr uint32_t inst_access_fault = 0x1;
        constexpr uint32_t illegal_inst = 0x2;
        constexpr uint32_t breakpoint = 0x3;
        constexpr uint32_t load_addr_misaligned = 0x4;
        constexpr uint32_t load_access_fault = 0x5;
        constexpr uint32_t store_addr_misaligned = 0x6;
        constexpr uint32_t store_access_fault = 0x7;
        constexpr uint32_t machine_ecall = 0xB; // 11
        //constexpr uint32_t software_check = 0x12; // 18
        constexpr uint32_t hardware_error = 0x13; // 19
        // interrupt codes (top bit set)
        constexpr uint32_t interrupt_bit = (1u << 31);
        namespace intr {
            constexpr uint32_t machine_sw = (interrupt_bit | 0x3u);
            constexpr uint32_t machine_timer = (interrupt_bit | 0x7u);
            constexpr uint32_t machine_ext = (interrupt_bit | 0xBu); // 11
            constexpr uint32_t machine_lcof = (interrupt_bit | 0xDu); // 13
        }
    }
}

// Instructions
namespace inst {
    constexpr uint32_t ecall = 0x00000073;
    constexpr uint32_t ebreak = 0x00100073;
    constexpr uint32_t mret = 0x30200073;
    constexpr uint32_t sret = 0x10200073;
    constexpr uint32_t wfi = 0x10500073;
    constexpr uint32_t fence_i = 0x0000100f;
    constexpr uint32_t nop = 0x00000013;
    constexpr uint32_t c_nop = 0x0001;

    namespace heuristic {
        constexpr uint32_t ret = 0x00008067; // jalr x0, 0(x1)
        constexpr uint32_t c_ret = 0x8082; // c.jr x1
        constexpr uint32_t ret_x5 = 0x00028067; // jalr x0, 0(x5)
        constexpr uint32_t ret_x15 = 0x00078067; // jalr x0, 0(x15)
    }

    namespace hint {
        constexpr uint32_t log_start = 0x01002013; // slti x0, x0, 0x10
        constexpr uint32_t log_end = 0x01102013; // slti x0, x0, 0x11
    }

    namespace align {
        #ifdef RV32C
        constexpr uint32_t pc_low_bits = 1u;
        constexpr uint32_t compressed_mask = 0xFFFF;
        #else
        constexpr uint32_t pc_low_bits = 2u;
        #endif
        constexpr uint32_t pc_low_bits_mask = ((1u << pc_low_bits) - 1u);
    }
};

// Decoder types
enum class opcode {
    d_alu_reg = 0b011'0011, // R type
    d_alu_imm = 0b001'0011, // I type
    d_load = 0b000'0011, // I type
    d_store = 0b010'0011, // S type
    d_branch = 0b110'0011, // B type
    d_jalr = 0b110'0111, // I type
    d_jal = 0b110'1111, // J type
    d_lui = 0b011'0111, // U type
    d_auipc = 0b001'0111, // U type
    d_system = 0b111'0011, // I type
    d_misc_mem = 0b000'1111, // I type
    d_custom_ext = 0b0000'1011
};

enum class alu_r_op_t {
    op_add = 0b0000,
    op_sub = 0b1000,
    op_sll = 0b0001,
    op_srl = 0b0101,
    op_sra = 0b1101,
    op_slt = 0b0010,
    op_sltu = 0b0011,
    op_xor = 0b0100,
    op_or = 0b0110,
    op_and = 0b0111
};

enum class alu_r_mul_op_t {
    op_mul = 0x0,
    op_mulh = 0x1,
    op_mulhsu = 0x2,
    op_mulhu = 0x3,
    op_div = 0x4,
    op_divu = 0x5,
    op_rem = 0x6,
    op_remu = 0x7,
};

enum class alu_r_zbb_op_t {
    op_max = 0x6,
    op_maxu = 0x7,
    op_min = 0x4,
    op_minu = 0x5,
};

enum class alu_i_op_t {
    op_addi = 0b0000,
    op_slli = 0b0001,
    op_srli = 0b0101,
    op_srai = 0b1101,
    op_slti = 0b0010,
    op_sltiu = 0b0011,
    op_xori = 0b0100,
    op_ori = 0b0110,
    op_andi = 0b0111
};

enum class load_op_t {
    op_lb = 0b000,
    op_lh = 0b001,
    op_lw = 0b010,
    op_lbu = 0b100,
    op_lhu = 0b101,
};

enum class store_op_t {
    op_sb = 0b000,
    op_sh = 0b001,
    op_sw = 0b010
};

enum class branch_op_t {
    op_beq = 0b000,
    op_bne = 0b001,
    op_blt = 0b100,
    op_bge = 0b101,
    op_bltu = 0b110,
    op_bgeu = 0b111
};

enum class csr_op_t {
    op_rw = 0b001,
    op_rs = 0b010,
    op_rc = 0b011,
    op_rwi = 0b101,
    op_rsi = 0b110,
    op_rci = 0b111
};

enum class custom_op_t {
    type_alu = 0x00,
    type_qalu = 0x01,
    type_mul = 0x02,
    type_wmul = 0x03,
    type_dot = 0x04,
    type_min_max = 0x08,
    type_shift = 0x09,
    type_data_fmt_widen = 0x20,
    type_data_fmt_narrow = 0x22,
    type_data_fmt_qnarrow = 0x23,
    type_data_fmt_txp = 0x30,
    type_sv_dup_vins = 0x3c,
    type_sv_vext = 0x3d,
    type_hints = 0x7f,
};

enum class alu_addsub_custom_op_t {
    op_add16 = 0x0,
    op_add8 = 0x2,
    op_sub16 = 0x4,
    op_sub8 = 0x6,
};

enum class alu_qaddsub_custom_op_t {
    op_qadd16 = 0x0,
    op_qadd16u = 0x1,
    op_qadd8 = 0x2,
    op_qadd8u = 0x3,
    op_qsub16 = 0x4,
    op_qsub16u = 0x5,
    op_qsub8 = 0x6,
    op_qsub8u = 0x7,
};

enum class alu_add_sub_op_t { add, sub };
enum class alu_min_max_op_t { min, max };
enum class alu_shift_op_t { left, right };

enum class alu_wmul_custom_op_t {
    op_wmul16 = 0x0,
    op_wmul16u = 0x1,
    op_wmul8 = 0x2,
    op_wmul8u = 0x3,
};

enum class alu_mul_custom_op_t {
    op_mul16   = 0x0,
    op_mul8    = 0x2,
    op_mulh16  = 0x4,
    op_mulh16u = 0x5,
    op_mulh8   = 0x6,
    op_mulh8u  = 0x7,
};

enum class alu_dot_custom_op_t {
    op_dot16 = 0x0,
    op_dot16u = 0x1,
    op_dot8 = 0x2,
    op_dot8u = 0x3,
    op_dot4 = 0x4,
    op_dot4u = 0x5,
    op_dot2 = 0x6,
    op_dot2u = 0x7,
};

enum class alu_min_max_custom_op_t {
    op_min16 = 0x0,
    op_min16u = 0x1,
    op_min8 = 0x2,
    op_min8u = 0x3,
    op_max16 = 0x4,
    op_max16u = 0x5,
    op_max8 = 0x6,
    op_max8u = 0x7,
};

enum class alu_shift_custom_op_t {
    op_slli16 = 0x0,
    op_slli8 = 0x2,
    op_srli16 = 0x4,
    op_srli8 = 0x6,
    op_srai16 = 0x5,
    op_srai8 = 0x7,
};

enum class data_fmt_widen_custom_op_t {
    op_widen16 = 0x6,
    op_widen16u = 0x7,
    op_widen8 = 0x0,
    op_widen8u = 0x1,
    op_widen4 = 0x2,
    op_widen4u = 0x3,
    op_widen2 = 0x4,
    op_widen2u = 0x5,
};

enum class data_fmt_narrow_custom_op_t {
    op_narrow32 = 0x0,
    op_narrow16 = 0x2,
    op_narrow8 = 0x4,
    op_narrow4 = 0x6,
};

enum class data_fmt_qnarrow_custom_op_t {
    op_qnarrow32 = 0x0,
    op_qnarrow32u = 0x1,
    op_qnarrow16 = 0x2,
    op_qnarrow16u = 0x3,
    op_qnarrow8 = 0x4,
    op_qnarrow8u = 0x5,
    op_qnarrow4 = 0x6,
    op_qnarrow4u = 0x7,
};

enum class data_fmt_txp_custom_op_t {
    op_txp16 = 0x0,
    op_txp8 = 0x2,
    op_txp4 = 0x4,
    op_txp2 = 0x6,
};

enum class sv_dup_custom_op_t {
    op_dup16 = 0x0,
    op_dup8 = 0x2,
    op_dup4 = 0x4,
    op_dup2 = 0x6,
};

enum class sv_vins_custom_op_t {
    op_vins16 = 0x1,
    op_vins8 = 0x3,
    op_vins4 = 0x5,
    op_vins2 = 0x7,
};

enum class sv_vext_custom_op_t {
    op_vext16 = 0x0,
    op_vext16u = 0x1,
    op_vext8 = 0x2,
    op_vext8u = 0x3,
    op_vext4 = 0x4,
    op_vext4u = 0x5,
    op_vext2 = 0x6,
    op_vext2u = 0x7,
};

enum class scp_custom_op_t {
    op_lcl = 0x0,
    op_rel = 0x1,
};

// not needed as isa sim can't log these
// enum class mhpmevent_t {
//     bad_spec = (1 << 0),
//     be = (1 << 1),
//     be_dc = (1 << 2),
//     fe = (1 << 3),
//     fe_ic = (1 << 4),
//     ret_simd = (1 << 5),
// };

enum class mem_op_t { read, write };
enum class b_dir_t { backward, forward };

// profilers
enum class profiler_source_t { inst, clock };

enum class dmem_size_t {
    lb, lh, lw, ld,
    sb, sh, sw, sd,
    no_access
};

struct clock_source_t {
    private:
        uint64_t pr = 0;
        uint64_t cr = 0;
        uint64_t diff = 0;
    public:
        void update(uint64_t sample) {
            pr = cr;
            cr = sample;
            diff = cr - pr;
        }
        uint64_t get_cr() { return cr; }
        uint64_t get_diff() { return diff; }
};

struct symbol_map_entry_t {
    uint16_t idx;
    std::string name;
};

struct symbol_lut_entry_t {
    uint32_t pc;
    std::string name;
};

struct symbol_tracking_t {
    std::vector<uint16_t> idx_callstack;
    std::vector<uint16_t> idx_callstack_prev;
    uint32_t fallthrough_pc;
    bool updated;
};

// perf events
enum class perf_event_t {
    inst,
    #ifdef DPI
    cycle,
    #endif
    branch,
    mem,
    //mem_load,
    //mem_store,
    simd,
    #if defined(HW_MODELS_EN) || defined(DPI)
    //cache_reference, // only applicable for multi-level caches
    //cache_miss, // only applicable for multi-level caches
    icache_reference,
    icache_miss,
    dcache_reference,
    dcache_miss,
    bp_mispredict,
    #endif
    _count
};

static const
std::array<std::string, static_cast<uint32_t>(perf_event_t::_count)>
perf_event_names = {
    "inst",
    #ifdef DPI
    "cycle",
    #endif
    "branch",
    "mem",
    //"mem_load",
    //"mem_store",
    "simd",
    #if defined(HW_MODELS_EN) || defined(DPI)
    //"cache_reference",
    //"cache_miss",
    "icache_reference",
    "icache_miss",
    "dcache_reference",
    "dcache_miss",
    "bp_mispredict",
    #endif
};

// hw models
namespace cache_cfg {
    constexpr uint32_t line_size = 64; // bytes
    constexpr uint32_t byte_addr_bits = (__builtin_ctz(line_size)); // 6
    constexpr uint32_t byte_addr_mask = (line_size - 1); // 0x3F, bottom 6 bits
    constexpr uint32_t max_sets = 1024;
    constexpr uint32_t max_ways = 128;
}

// dasm
struct dasm_str {
    public:
        std::ostringstream asm_ss;
        std::ostringstream simd_ss;
        std::ostringstream simd_a;
        std::ostringstream simd_b;
        std::ostringstream simd_c;
        std::string asm_str;
        std::string op;
    public:
        void finish_inst() {
            asm_ss << simd_ss.str();
            asm_str = asm_ss.str();
        }
        void clear_str() {
            asm_ss.str("");
            simd_ss.str("");
        }
};

// RF
enum class rf_names_t { mode_x, mode_abi };

struct reg_pair {
    uint32_t a;
    uint32_t b;
};

// CSRs
struct CSR {
    const char* name;
    uint32_t value;
    const csrm::perm_t perm;
    const uint32_t wmask; // writable bits; 0-bits are hardwired to boot value
    CSR(
        const char* name,
        uint32_t value,
        const csrm::perm_t perm,
        uint32_t wmask
    ) : name(name), value(value), perm(perm), wmask(wmask) {}
};

struct CSR_entry {
    const uint16_t csr_addr;
    const char* csr_name;
    const csrm::perm_t perm;
    const uint32_t boot_val;
    const uint32_t wmask = 0xFFFF'FFFF; // fully writable by default
};

// CLI options
struct prof_pc_t {
    public:
        uint32_t start;
        uint32_t stop;
        uint32_t single_match_num;
        uint64_t inst_cnt = 0;
        bool exit_on_prof_stop;
    private:
        uint32_t current_match = 0;
        bool ran_once = false;
    public:
        bool should_start() {
            if (!single_match_num) return true; // dc if not single matching
            current_match++;
            if (current_match == single_match_num) { // count matched
                ran_once = true;
                return true;
            }
            return false; // single matching, but count not yet matched
        }
        bool should_start(uint32_t pc) {
            if (!(pc == start)) return false;
            return should_start();
        }
        bool should_stop(uint32_t pc) {
            if (pc == stop) return true;
            return false;
        }
        bool should_exit() {
            if (!exit_on_prof_stop) return false; // dc if inactive
            if (!single_match_num) return true;// exit on first stop if no match
            if (ran_once) return true; // exit if single match and ran once
            return false; // single matching, but not run yet
        }
        bool should_exit(uint32_t pc) {
            if (pc != stop) return false; // dc if not at stop pc
            return should_exit();
        }
};

struct cfg_t {
    prof_pc_t prof_pc;
    rf_names_t rf_names;
    uint32_t mem_dump_start;
    uint32_t mem_dump_size;
    perf_event_t perf_event;
    uint64_t run_insts;
    uint64_t run_steps;
    bool prof_trace;
    bool rf_usage;
    bool no_callstack;
    bool log;
    bool log_always;
    bool log_state;
    bool log_hw_models;
    bool show_state;
    bool exit_on_trap;
    bool print_uart;
    std::string uart_in;
    bool silent;
    std::string out_dir;
};

struct logging_flags_t {
    bool en;
    bool act;
    bool always;
    bool state;
    void activate(bool in) { act = en && (in || always); }
};

#pragma once

// UART RX input (UART_IN=1) requires the UART device
#if defined(UART_INPUT_EN) && !defined(UART_EN)
#error "UART_INPUT_EN requires UART_EN"
#endif

#ifdef DPI
#ifdef HW_MODELS_EN
#error "HW models are not allowed in DPI mode"
#endif
#ifndef PROFILERS_EN
#error "DPI requires profilers"
#endif
#ifndef DASM_EN
#error "DPI requires DASM"
#endif
#ifndef UART_EN
#error "DPI requires UART"
#endif
#endif

#include "types.h"

// casts
#define TO_F64(x) static_cast<double_t>((x))
#define TO_F32(x) static_cast<float_t>((x))
#define TO_U64(x) static_cast<uint64_t>((x))
#define TO_I64(x) static_cast<int64_t>((x))
#define TO_U32(x) static_cast<uint32_t>((x))
#define TO_I32(x) static_cast<int32_t>((x))
#define TO_U16(x) static_cast<uint16_t>((x))
#define TO_I16(x) static_cast<int16_t>((x))
#define TO_U8(x) static_cast<uint8_t>((x))
#define TO_I8(x) static_cast<int8_t>((x))
//#define TO_U4(x) static_cast<uint8_t>((x) & 0xF)
//#define TO_I4(x) static_cast<int8_t>(((x) & 0xF) | (((x) & 0x8) ? 0xF0 :0x00))
//#define TO_U2(x) static_cast<uint8_t>((x) & 0x3)
//#define TO_I2(x) static_cast<int8_t>(((x) & 0x3) | (((x) & 0x2) ? 0xFC :0x00))
#define TO_BOOL(x) static_cast<bool>((x))

// HW models
#define CACHE_MODE_PERF 0 // tags and stats
#define CACHE_MODE_FUNC 1 // adds data

#ifndef CACHE_MODE
#define CACHE_MODE CACHE_MODE_FUNC
#endif
// optionally, compare against isa sim, only for CACHE_MODE_FUNC
//#define CACHE_VERIFY

// Instruction field masks
#define M_OPC7 TO_U32(0x7F)
#define M_OPC2 TO_U32(0x3)

#define M_FUNCT7 TO_U32((0x7F)<<25)
#define M_FUNCT7_B5 TO_U32((0x1)<<30)
#define M_FUNCT7_B1 TO_U32((0x1)<<25)
#define M_FUNCT3 TO_U32((0x7)<<12)
#define M_CFUNCT2H TO_U32((0x3)<<10)
#define M_CFUNCT2L TO_U32((0x3)<<5)
#define M_CFUNCT3 TO_U32((0x7)<<13)
#define M_CFUNCT4 TO_U32((0xF)<<12)
#define M_CFUNCT6 TO_U32((0x3F)<<10)

#define M_RD TO_U32((0x1F)<<7)
#define M_RS1 TO_U32((0x1F)<<15)
#define M_RS2 TO_U32((0x1F)<<20)
#define M_CRS2 TO_U32((0x1F)<<2)
#define M_CREGH TO_U32((0x7)<<7)
#define M_CREGL TO_U32((0x7)<<2)
#define M_IMM_SHAMT TO_U32(0x1F)

#define M_IMM_31_25 TO_I32((0x7F)<<25)
#define M_IMM_31_20 TO_I32((0xFFF)<<20)
#define M_IMM_30_25 TO_U32((0x3F)<<25)
#define M_IMM_24_21 TO_U32((0xF)<<21)
#define M_IMM_24_20 TO_U32((0x1F)<<20)
#define M_IMM_19_12 TO_U32((0xFF)<<12)
#define M_IMM_12_11 TO_U32((0x3)<<11)
#define M_IMM_12_10 TO_U32((0x7)<<10)
#define M_IMM_12_9 TO_U32((0xF)<<9)
#define M_IMM_11_10 TO_U32((0x3)<<10)
#define M_IMM_11_8 TO_U32((0xF)<<8)
#define M_IMM_10_9 TO_U32((0x3)<<9)
#define M_IMM_10_7 TO_U32((0xF)<<7)
#define M_IMM_8_7 TO_U32((0x3)<<7)
#define M_IMM_6_5 TO_U32((0x3)<<5)
#define M_IMM_6_4 TO_U32((0x7)<<4)
#define M_IMM_6_2 TO_U32((0x1F)<<2)
#define M_IMM_5_3 TO_U32((0x7)<<3)
#define M_IMM_4_3 TO_U32((0x3)<<3)
#define M_IMM_3_2 TO_U32((0x3)<<2)

#define M_IMM_31 TO_I32((0x1)<<31)
#define M_IMM_20 TO_U32((0x1)<<20)
#define M_IMM_12 TO_U32((0x1)<<12)
#define M_IMM_11 TO_U32((0x1)<<11)
#define M_IMM_8 TO_U32((0x1)<<8)
#define M_IMM_7 TO_U32((0x1)<<7)
#define M_IMM_6 TO_U32((0x1)<<6)
#define M_IMM_5 TO_U32((0x1)<<5)
#define M_IMM_2 TO_U32((0x1)<<2)

// Macros
#define CASE_DECODER(op) \
    case TO_U8(opcode::op): \
        op(); \
        break;

#define CASE_ALU_REG_OP(op) \
    case TO_U8(alu_r_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], rf[ip.rs2()], alu) \
        res = alu_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_ALU_REG_MUL_OP(op) \
    case TO_U8(alu_r_mul_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], rf[ip.rs2()], mul) \
        res = alu_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_SET_PERF_EVENT_MUL \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#ifdef HW_MODELS_EN
#define DIV_HM_EVAL(uns) div.eval(rf[ip.rs1()], rf[ip.rs2()], uns);
#else
#define DIV_HM_EVAL(uns)
#endif

#define CASE_ALU_REG_DIV_OP(op, uns) \
    case TO_U8(alu_r_mul_op_t::op_##op): \
        DIV_HM_EVAL(uns) \
        PROF_SPARSITY(rf[ip.rs1()], 1u, div_a) \
        PROF_SPARSITY(1u, rf[ip.rs2()], div_b) \
        res = alu_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_SET_PERF_EVENT_DIV \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_ALU_REG_ZBB_OP(op) \
    case TO_U8(alu_r_zbb_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], rf[ip.rs2()], alu) \
        res = alu_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_ALU_IMM_OP(op) \
    case TO_U8(alu_i_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], ip.imm_i(), alu) \
        res = alu_##op(rf[ip.rs1()], ip.imm_i()); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1 \
        PROF_RD_ZERO(res) \
        break;

#ifdef DPI
#define MMIO_RTL_TRUSTED_OVERRIDE(val, addr) \
    if (mmio_rtl_trusted(addr)) val = get_rtl_rf_value(ip.rd());
#else
#define MMIO_RTL_TRUSTED_OVERRIDE(val, addr)
#endif

#define CASE_LOAD(op) \
    case TO_U8(load_op_t::op_##op): { \
        uint32_t mmio_addr = rf[ip.rs1()] + ip.imm_i(); \
        loaded = load_##op(mmio_addr); \
        if (tu.is_trapped()) return; \
        MMIO_RTL_TRUSTED_OVERRIDE(loaded, mmio_addr) \
        PROF_SPARSITY(loaded, 1u, mem_l) \
        write_rf(ip.rd(), loaded); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1 \
        PROF_RD_ZERO(loaded) \
        } break;

#define CASE_STORE(op) \
    case TO_U8(store_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs2()], 1u, mem_s) \
        addr = (rf[ip.rs1()] + ip.imm_s()); \
        store_##op(addr, rf[ip.rs2()]); \
        if (tu.is_trapped()) return; \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RS1_RS2 \
        break;

#define CASE_BRANCH(op) \
    case TO_U8(branch_op_t::op_##op): \
        if(branch_##op()) { \
            next_pc = target_pc; \
            taken = true; \
            PROF_B_T(op) \
        } else { \
            PROF_B_NT(op, target_pc) \
        } \
        DASM_OP(op) \
        PROF_RS1_RS2 \
        break;

#define CASE_ADDSUB_CUSTOM_OP(op, t) \
    case TO_U8(alu_addsub_custom_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], rf[ip.rs2()], simd_##t) \
        res = alu_c_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_QADDSUB_CUSTOM_OP(op, t) \
    case TO_U8(alu_qaddsub_custom_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], rf[ip.rs2()], simd_##t) \
        res = alu_c_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_WMUL_CUSTOM_OP(op, t) \
    case TO_U8(alu_wmul_custom_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], rf[ip.rs2()], simd_##t) \
        rp = alu_c_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf_pair(ip.rd(), rp); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RDP_RS1_RS2 \
        PROF_RD_ZERO(rp.a) \
        PROF_RDP_ZERO(rp.b) \
        break;

#define CASE_MUL_CUSTOM_OP(op, t) \
    case TO_U8(alu_mul_custom_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], rf[ip.rs2()], simd_##t) \
        res = alu_c_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_ALU_CUSTOM_DOT(op, t) \
    case TO_U8(alu_dot_custom_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], rf[ip.rs2()], simd_##t) \
        res = alu_c_##op(rf[ip.rs1()], rf[ip.rs2()], rf[ip.rd()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2_RS3 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_MIN_MAX_CUSTOM_OP(op, t) \
    case TO_U8(alu_min_max_custom_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], rf[ip.rs2()], simd_##t) \
        res = alu_c_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_SHIFT_CUSTOM_OP(op, t) \
    case TO_U8(alu_shift_custom_op_t::op_##op): \
        PROF_SPARSITY(rf[ip.rs1()], shamt, simd_##t) \
        res = alu_c_##op(rf[ip.rs1()], shamt); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_DATA_FMT_WIDEN_CUSTOM_OP(op) \
    case TO_U8(data_fmt_widen_custom_op_t::op_##op): \
        rp = data_fmt_c_##op(rf[ip.rs1()], shamt); \
        write_rf_pair(ip.rd(), rp); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RDP_RS1 \
        PROF_RD_ZERO(rp.a) \
        PROF_RDP_ZERO(rp.b) \
        break;

#define CASE_DATA_FMT_NARROW_CUSTOM_OP(op) \
    case TO_U8(data_fmt_narrow_custom_op_t::op_##op): \
        res = data_fmt_c_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_DATA_FMT_QNARROW_CUSTOM_OP(op) \
    case TO_U8(data_fmt_qnarrow_custom_op_t::op_##op): \
        res = data_fmt_c_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1_RS2 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_DATA_FMT_TXP_CUSTOM_OP(op) \
    case TO_U8(data_fmt_txp_custom_op_t::op_##op): \
        rp = data_fmt_c_##op(rf[ip.rs1()], rf[ip.rs2()]); \
        write_rf_pair(ip.rd(), rp); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RDP_RS1_RS2 \
        PROF_RD_ZERO(rp.a) \
        PROF_RDP_ZERO(rp.b) \
        break;

#define CASE_SV_DUP_CUSTOM_OP(op) \
    case TO_U8(sv_dup_custom_op_t::op_##op): \
        res = data_fmt_c_##op(rf[ip.rs1()]); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_SV_VINS_CUSTOM_OP(op, lane_mask) \
    case TO_U8(sv_vins_custom_op_t::op_##op): \
        res = data_fmt_c_##op( \
            rf[ip.rd()], rf[ip.rs1()], TO_U8(ip.rs2() & (lane_mask))); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_SV_VEXT_CUSTOM_OP(op, lane_mask) \
    case TO_U8(sv_vext_custom_op_t::op_##op): \
        res = data_fmt_c_##op(rf[ip.rs1()], TO_U8(ip.rs2() & (lane_mask))); \
        write_rf(ip.rd(), res); \
        DASM_OP(op) \
        PROF_G(op) \
        PROF_RD_RS1 \
        PROF_RD_ZERO(res) \
        break;

#define CASE_SCP_CUSTOM(op) \
    case TO_U8(scp_custom_op_t::op_##op): \
        write_rf(ip.rd(), \
                 TO_U32(mem->cache_hint(rf[ip.rs1()], scp_mode_t::m_##op))); \
        PROF_G(scp_##op) \
        PROF_RD_RS1 \

#define CASE_CSR(op) \
    case TO_U8(csr_op_t::op_##op): \
        csr_##op(init_val_rs1); \
        DASM_OP(csr##op) \
        PROF_G(csr##op) \
        PROF_RD_RS1 \
        DASM_CSR_REG \
        break;

#define CASE_CSR_I(op) \
    case TO_U8(csr_op_t::op_##op): \
        csr_##op(); \
        DASM_OP(csr##op) \
        PROF_G(csr##op) \
        PROF_RD \
        DASM_CSR_IMM \
        break;

#define W_CSR(expr) write_csr(ip.csr_addr(), expr)

#define SIM_ERROR std::cerr << "\n >> SIM RUNTIME ERROR: "
#define SIM_WARNING std::cout << "\n >> SIM RUNTIME WARNING: "
#define DASM_TRAP dasm.asm_ss << "Instruction trapped: "

#define BIN_FORMAT(val, n) \
    std::setw((n)) << std::setfill('0') << std::bitset<n>(val) << std::dec

#define MEM_ADDR_FORMAT(addr) \
    std::setw(mem_addr_bitwidth) << std::setfill('0') << std::hex << addr \
                                 << std::dec

#define INST_FORMAT(inst, n) \
    std::setw((n)) << std::setfill('0') << std::hex << inst << std::dec

#define FORMAT_INST(pc, inst, n) \
    MEM_ADDR_FORMAT(pc) << ": " << INST_FORMAT((inst), n)

#define FHEXZ(val, w) \
    "0x" << std::setw(w) << std::setfill('0') << std::hex << val << std::dec

#define FHEXN(val, w) \
    "0x" << std::left << std::setw(w) << std::setfill(' ') << std::hex \
         << val << std::dec

// Format Register File print
#define FRF(addr, val) \
    std::left << std::setw(rf_names_w) << std::setfill(' ') << addr \
              << ": 0x" << std::right << std::setw(8) << std::setfill('0') \
              << std::hex << val << std::dec

// Format CSR print
#define CSRF(it) \
    std::hex << "0x" << std::setw(4) << std::setfill('0') << it->first << " " \
             << std::left << std::setw(csr_names_w) << std::setfill(' ') \
             << it->second.name << ": 0x" \
             << std::right << std::setw(8) << std::setfill('0') \
             << it->second.value << std::dec << std::setfill(' ')

#ifdef DASM_EN
// FIXME: need to differentiate names between macros that redirect to dasm and
// those that are just formatting string or accessing registers

#ifdef PROFILERS_EN
#define LOG_SYMBOL_TO_FILE log_ofstream << prof_perf.get_callstack_str() << "\n"
#else
#define LOG_SYMBOL_TO_FILE
#endif

#define DASM_OP(o) dasm.op = #o;

#define DASM_CSR_REG \
    dasm.asm_ss << dasm.op << " " << rf_names[ip.rd()][rf_names_idx] << "," \
                << csr.at(ip.csr_addr()).name << "," \
                << rf_names[ip.rs1()][rf_names_idx];

#define DASM_CSR_IMM \
    dasm.asm_ss << dasm.op << " " << rf_names[ip.rd()][rf_names_idx] << "," \
                << csr.at(ip.csr_addr()).name << "," \
                << ip.uimm_csr();

#define DASM_OP_RD \
    dasm.asm_ss << dasm.op << " " << rf_names[ip.rd()][rf_names_idx]

#define DASM_OP_RS1 rf_names[ip.rs1()][rf_names_idx]
#define DASM_OP_RS2 rf_names[ip.rs2()][rf_names_idx]

#define DASM_OP_CREGH \
    dasm.asm_ss << dasm.op << " " << rf_names[ip.c_regh()][rf_names_idx]

#define DASM_CREGL \
    rf_names[ip.c_regl()][rf_names_idx]

#define DASM_ALIGN \
    dasm.asm_ss << std::setw(38 - inst_w - dasm.asm_ss.tellp()) \
                << std::setfill(' ') << "  "

// parametrized
#define DASM_RD_UPDATE_P(rd) \
    if (rd) DASM_ALIGN << FRF(rf_names[rd][rf_names_idx], rf[rd]);

// generic
#define DASM_RD_UPDATE \
    { \
        DASM_RD_UPDATE_P(ip.rd()) \
    }

#define DASM_RD_UPDATE_PAIR \
    { \
        if (ip.rd()) { \
            dasm.asm_ss << ", "; \
            DASM_RD_UPDATE_P(ip.rd() + 1) \
        } \
    }

inline std::string dasm_ascii_hint(int32_t val, uint32_t addr) {
    if ((addr != mem_map::uart0_tx_data_addr) &&
        (addr != mem_map::uart0_rx_data_addr))
    {
        return "";
    }
    if (val >= 0x20 && val <= 0x7e) { // printable ASCII characters
        return std::string(" # '") + char(val) + "'";
    }
    if (val == '\n') return " # '\\n'";
    if (val == '\r') return " # '\\r'";
    if (val == '\t') return " # '\\t'";
    return "";
}

// parametrized
#define DASM_MEM_UPDATE_P(addr, rs) \
    DASM_ALIGN << "mem[" \
               << MEM_ADDR_FORMAT(addr) \
               << "] <- " << rf_names[rs][rf_names_idx] \
               << " (" << FHEXZ(rf[rs], 8) << ")" \
               << dasm_ascii_hint(rf[rs], (addr))

// generic
#define DASM_MEM_UPDATE \
    DASM_MEM_UPDATE_P(TO_I32(rf[ip.rs1()] + ip.imm_s()), ip.rs2())

#else
#define LOG_SYMBOL_TO_FILE
#define DASM_OP(o)
#define DASM_CSR_REG
#define DASM_CSR_IMM
#define DASM_OP_RD
#define DASM_OP_RS1
#define DASM_OP_RS2
#define DASM_OP_CREGH
#define DASM_CREGL
#define DASM_RD_UPDATE_P(rd)
#define DASM_RD_UPDATE
#define DASM_MEM_UPDATE_P(addr, rs)
#define DASM_MEM_UPDATE
#endif

#if defined(DASM_EN) || defined(PROFILERS_EN)
#define INST_HEX_W(x) \
    inst_w = x;
#else
#define INST_HEX_W(x)
#endif

#ifdef PROFILERS_EN

#ifdef DPI
#define DIFF clk_src.get_diff()
#define PROF_SRC profiler_source_t::clock
#else
#define DIFF 1
#define PROF_SRC profiler_source_t::inst
#endif

#define PROF_G(op) \
    prof.log_inst(opc_g::i_##op, DIFF); \
    prof_rf.te.opc_g_val = TO_U8(opc_g::i_##op);

#define PROF_J(op) \
    b_dir_t dir = (next_pc > pc) ? b_dir_t::forward : b_dir_t::backward; \
    prof.log_inst(opc_b::i_##op, true, b_dir_t(dir), DIFF); \
    prof_rf.te.opc_b_val = TO_U8(opc_b::i_##op);

#define PROF_B_T(op) \
    b_dir_t dir = (next_pc > pc) ? b_dir_t::forward : b_dir_t::backward; \
    prof.log_inst(opc_b::i_##op, true, b_dir_t(dir), DIFF); \
    prof_rf.te.opc_b_val = TO_U8(opc_b::i_##op);

#define PROF_B_NT(op, t_pc) \
    b_dir_t dir = (t_pc > pc) ? b_dir_t::forward : b_dir_t::backward; \
    prof.log_inst(opc_b::i_##op, false, b_dir_t(dir), DIFF); \
    prof_rf.te.opc_b_val = TO_U8(opc_b::i_##op);

#define PROF_RD \
    prof_rf.log_reg_use(reg_use_t::rd, ip.rd()); \
    prof_rf.te.rd = ip.rd();

#define PROF_RDP \
    prof_rf.log_reg_use(reg_use_t::rdp, ip.rd()+1);

#define PROF_RS1 \
    prof_rf.log_reg_use(reg_use_t::rs1, ip.rs1()); \
    prof_rf.te.rs1 = ip.rs1();

#define PROF_RS2 \
    prof_rf.log_reg_use(reg_use_t::rs2, ip.rs2()); \
    prof_rf.te.rs2 = ip.rs2();

#define PROF_RD_ZERO(val) \
    prof_rf.te.rd_val_zero = ((val) == 0) ? 1u : 0u;

#define PROF_RDP_ZERO(val) \
    prof_rf.te.rdp_val_zero = ((val) == 0) ? 1u : 0u;

#define PROF_C_RD_REGH \
    prof_rf.log_reg_use(reg_use_t::rd,  ip.c_regh()); \
    prof_rf.te.rd  = ip.c_regh();

#define PROF_C_RD_REGL \
    prof_rf.log_reg_use(reg_use_t::rd,  ip.c_regl()); \
    prof_rf.te.rd  = ip.c_regl();

#define PROF_C_RS1_RD \
    prof_rf.log_reg_use(reg_use_t::rs1, ip.rd()); \
    prof_rf.te.rs1 = ip.rd();

#define PROF_C_RS1_REGH \
    prof_rf.log_reg_use(reg_use_t::rs1, ip.c_regh()); \
    prof_rf.te.rs1 = ip.c_regh();

#define PROF_C_RS2_REGL \
    prof_rf.log_reg_use(reg_use_t::rs2, ip.c_regl()); \
    prof_rf.te.rs2 = ip.c_regl();

#define PROF_C_RS2_RS2 \
    prof_rf.log_reg_use(reg_use_t::rs2, ip.c_rs2()); \
    prof_rf.te.rs2 = ip.c_rs2();

#define PROF_C_RD_LIT(n) \
    prof_rf.log_reg_use(reg_use_t::rd,  (n)); \
    prof_rf.te.rd  = (n);

#define PROF_C_RS1_LIT(n) \
    prof_rf.log_reg_use(reg_use_t::rs1, (n)); \
    prof_rf.te.rs1 = (n);

#define PROF_RS3 \
    prof_rf.log_reg_use(reg_use_t::rs3, ip.rd());

#define PROF_RD_RS1_RS2 \
    PROF_RD \
    PROF_RS1 \
    PROF_RS2

#define PROF_RD_RS1_RS2_RS3 \
    PROF_RD \
    PROF_RS1 \
    PROF_RS2 \
    PROF_RS3

#define PROF_RD_RDP_RS1_RS2 \
    PROF_RD \
    PROF_RDP \
    PROF_RS1 \
    PROF_RS2

#define PROF_RD_RDP_RS1 \
    PROF_RD \
    PROF_RDP \
    PROF_RS1

#define PROF_RD_RS1 \
    PROF_RD \
    PROF_RS1

#define PROF_RS1_RS2 \
    PROF_RS1 \
    PROF_RS2

#define PROF_DMEM(size) \
    prof.te.dmem_size = TO_U8(size); \
    prof.te.dmem = addr;

// cosim collects these from RTL, don't count on the isa sim side
#ifndef DPI
#define PROF_SET_PERF_EVENT_SIMD \
    prof_perf.set_perf_event_flag(perf_event_t::ret_simd);
#define PROF_SET_PERF_EVENT_SIMD_ARITH \
    prof_perf.set_perf_event_flag(perf_event_t::ret_simd_arith);
#define PROF_SET_PERF_EVENT_SIMD_ARITH_DOT \
    prof_perf.set_perf_event_flag(perf_event_t::ret_simd_arith_dot);
#define PROF_SET_PERF_EVENT_MUL \
    prof_perf.set_perf_event_flag(perf_event_t::ret_mul);
#define PROF_SET_PERF_EVENT_DIV \
    prof_perf.set_perf_event_flag(perf_event_t::ret_div);
#define PROF_SET_PERF_EVENT_CTRL_FLOW \
    prof_perf.set_perf_event_flag(perf_event_t::ret_ctrl_flow);
#define PROF_SET_PERF_EVENT_CTRL_FLOW_JR \
    prof_perf.set_perf_event_flag(perf_event_t::ret_ctrl_flow_jr);
#define PROF_SET_PERF_EVENT_CTRL_FLOW_BR \
    prof_perf.set_perf_event_flag(perf_event_t::ret_ctrl_flow_br);
#define PROF_SET_PERF_EVENT_MEM \
    prof_perf.set_perf_event_flag(perf_event_t::ret_mem);
#define PROF_SET_PERF_EVENT_MEM_LOAD \
    prof_perf.set_perf_event_flag(perf_event_t::ret_mem_load);

#else
#define PROF_SET_PERF_EVENT_SIMD
#define PROF_SET_PERF_EVENT_SIMD_ARITH
#define PROF_SET_PERF_EVENT_SIMD_ARITH_DOT
#define PROF_SET_PERF_EVENT_MUL
#define PROF_SET_PERF_EVENT_DIV
#define PROF_SET_PERF_EVENT_CTRL_FLOW
#define PROF_SET_PERF_EVENT_CTRL_FLOW_JR
#define PROF_SET_PERF_EVENT_CTRL_FLOW_BR
#define PROF_SET_PERF_EVENT_MEM
#define PROF_SET_PERF_EVENT_MEM_LOAD
#endif // DPI

#define PROF_SPARSITY_ANY(res) \
    prof.log_sparsity((res == 0), sparsity_t::sp_any);
#define PROF_SPARSITY(a, b, cls) \
    prof.log_sparsity(((a == 0) || (b == 0)), sparsity_t::sp_##cls);

#else // !PROFILERS_EN
#define PROF_G(op)
#define PROF_J(op)
#define PROF_B_T(op)
#define PROF_B_NT(op, b)
#define PROF_RD
#define PROF_RDP
#define PROF_RS1
#define PROF_RS2
#define PROF_RS3
#define PROF_RD_RS1_RS2
#define PROF_RD_RDP_RS1_RS2
#define PROF_RD_RS1_RS2_RS3
#define PROF_RD_RDP_RS1
#define PROF_RD_RS1
#define PROF_RS1_RS2
#define PROF_DMEM(addr)
#define PROF_SET_PERF_EVENT_SIMD
#define PROF_SET_PERF_EVENT_SIMD_ARITH
#define PROF_SET_PERF_EVENT_SIMD_ARITH_DOT
#define PROF_SET_PERF_EVENT_MUL
#define PROF_SET_PERF_EVENT_DIV
#define PROF_SET_PERF_EVENT_CTRL_FLOW
#define PROF_SET_PERF_EVENT_CTRL_FLOW_JR
#define PROF_SET_PERF_EVENT_CTRL_FLOW_BR
#define PROF_SET_PERF_EVENT_MEM
#define PROF_SET_PERF_EVENT_MEM_LOAD
#define PROF_SPARSITY_ANY(res)
#define PROF_SPARSITY(a, b, cls)
#define PROF_RD_ZERO(val)
#define PROF_RDP_ZERO(val)
#define PROF_C_RD_REGH
#define PROF_C_RD_REGL
#define PROF_C_RS1_RD
#define PROF_C_RS1_REGH
#define PROF_C_RS2_REGL
#define PROF_C_RS2_RS2
#define PROF_C_RD_LIT(n)
#define PROF_C_RS1_LIT(n)
#endif // PROFILERS_EN

#define INDENT "    "
#define INDENT_3X INDENT << INDENT << INDENT

#define JSON_N "\n    "

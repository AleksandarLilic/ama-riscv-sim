#pragma once

#include <string>
#include <vector>

#include "profiler.h"

static_assert(TO_U32(opc_g::_count) <= 0xFF, "opc_g exceeds uint8_t");
static_assert(TO_U32(opc_b::_count) <= 0xFF, "opc_b exceeds uint8_t");

struct rf_trace_entry {
    static constexpr uint8_t OPC_NONE = 0xFF;
    static constexpr uint8_t REG_NONE = 0xFF;
    uint8_t opc_g_val; // TO_U8(opc_g::i_##op), OPC_NONE if branch/jump
    uint8_t opc_b_val; // TO_U8(opc_b::i_##op), OPC_NONE if general inst
    uint8_t rd; // destination register index, REG_NONE if no rd
    uint8_t rs1; // source register 1 index, REG_NONE if no rs1
    uint8_t rs2; // source register 2 index, REG_NONE if no rs2
    uint8_t rd_val_zero; // 1 if written rd value == 0, else 0
    uint8_t rdp_val_zero; // 1 if written rdp value == 0, else 0
    void rst() {
        opc_g_val = OPC_NONE;
        opc_b_val = OPC_NONE;
        rd = REG_NONE;
        rs1 = REG_NONE;
        rs2 = REG_NONE;
        rd_val_zero = 0;
        rdp_val_zero = 0;
    }
};
static_assert(sizeof(rf_trace_entry) == 7, "rf_trace_entry layout changed");

class profiler_rf {
public:
    rf_trace_entry te;

private:
    bool active = false;
    bool trace_en = false;
    bool rf_usage_en = false;

public:
    profiler_rf() { te.rst(); }
    void add_te();
    void log_reg_use(reg_use_t reg_use, uint8_t reg);
    void finish(const std::string& out_dir);
    void set_active(bool active) { this->active = active; te.rst(); }
    void set_trace_en(bool trace_en) { this->trace_en = trace_en; }
    void set_rf_usage_en(bool rf_usage_en) { this->rf_usage_en = rf_usage_en; }

private:
    std::vector<rf_trace_entry> trace;
    std::array<std::array<uint64_t, TO_U32(reg_use_t::_count)>, 32>
        prof_rf_usage = {{}};
};

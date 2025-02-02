#pragma once

#include "defines.h"

class profiler_perf {
    public:
        bool active;

    private:
        std::string log_path;
        symbol_tracking_t st;
        std::map<uint32_t, symbol_map_entry_t> symbol_map;
        std::vector<symbol_lut_entry_t> symbol_lut;
        perf_event_t perf_event;
        std::array<uint8_t, TO_U32(perf_event_t::_count)> perf_event_flags;
        uint64_t callstack_cnt;
        std::unordered_map<std::string, uint64_t> callstack_cnt_map;

    public:
        profiler_perf() = delete;
        profiler_perf(
            std::string log_path,
            std::map<uint32_t, symbol_map_entry_t> symbol_map,
            perf_event_t perf_event);
        bool finish_inst(uint32_t next_pc);
        std::string get_callstack_str() {
            return get_callstack_str(st.idx_callstack);
        }
        void update_branch(uint32_t next_pc, bool taken);
        void update_jalr(uint32_t next_pc, bool inst_ret);
        void update_jal(uint32_t next_pc, bool tail_call);
        void set_perf_event_flag(perf_event_t perf_event) {
            perf_event_flags[TO_U32(perf_event)] += 1;
        }
        void finish() { log_to_file(); }

    private:
        void inc_callstack_cnt();
        void save_callstack_cnt();
        void callstack_empty_check(const std::string& inst, uint32_t next_pc);
        void update_callstack(uint32_t cr_pc);
        void set_fallthrough_symbol(uint32_t cr_pc);
        bool symbol_change_on_jump(uint32_t next_pc);
        std::string get_callstack_str(std::vector<uint8_t> idx_stack);
        std::string callstack_to_key();
        void log_to_file();
};

#pragma once

#include "defines.h"

class profiler_perf {
    private:
        bool active;
        std::string out_dir;
        symbol_tracking_t st;
        std::map<uint32_t, symbol_map_entry_t> symbol_map;
        std::vector<symbol_lut_entry_t> symbol_lut;
        std::vector<perf_event_t> perf_events;
        profiler_source_t prof_src;
        std::array<uint8_t, TO_U32(perf_event_t::_count)> perf_event_flags;
        std::array<uint64_t, TO_U32(perf_event_t::_count)> callstack_cnt;
        std::unordered_map<
            std::u16string,
            std::array<uint64_t, TO_U32(perf_event_t::_count)>
        > callstack_cnt_map;
        #ifdef DPI
        clock_source_t* clk_src;
        #endif
        uint32_t diverged_cnt = 0;
        bool callstack_en = true;

    public:
        profiler_perf() = delete;
        profiler_perf(
            std::string out_dir,
            std::map<uint32_t, symbol_map_entry_t> symbol_map,
            std::vector<perf_event_t> perf_events,
            profiler_source_t prof_src);
        #ifdef DPI
        void set_clk_src (clock_source_t* src) { clk_src = src; }
        #endif
        void set_active(bool active) { this->active = active; }
        void set_callstack_en(bool en) { callstack_en = en; }
        bool is_callstack_en() const { return callstack_en; }
        bool finish_inst(uint32_t next_pc);
        std::string get_callstack_str() {
            return get_callstack_str(st.idx_callstack);
        }
        std::string get_callstack_top_str() {
            return get_callstack_str({st.idx_callstack.back()});
        }
        void update_branch(uint32_t next_pc, bool taken);
        void update_jalr(
            uint32_t next_pc, bool inst_ret, bool tail_call, uint32_t ra);
        void update_jal(uint32_t next_pc, bool tail_call, uint32_t ra);
        void set_perf_event_flag(perf_event_t perf_event) {
            perf_event_flags[TO_U32(perf_event)] += 1;
        }
        void finish(bool show) { log_to_file_and_print(show); }
        bool match_top(uint32_t next_pc);

    private:
        void inc_callstack_cnt();
        void save_callstack_cnt();
        void catch_empty_callstack(const std::string& inst, uint32_t next_pc);
        void update_callstack(uint32_t next_pc);
        void set_fallthrough_symbol(uint32_t pc);
        bool symbol_change_on_jump(uint32_t next_pc);
        bool match_symbol(uint32_t pc, uint16_t idx);
        std::optional<std::pair<uint32_t, symbol_map_entry_t>>
            find_symbol_in_range(uint32_t next_pc);
        std::string get_callstack_str(const std::vector<uint16_t>& idx_stack);
        std::u16string callstack_to_key();
        void log_to_file_and_print(bool show);
};

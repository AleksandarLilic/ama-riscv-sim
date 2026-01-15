
#include "profiler_perf.h"

profiler_perf::profiler_perf(
    std::string out_dir,
    std::map<uint32_t, symbol_map_entry_t> symbol_map,
    perf_event_t perf_event,
    profiler_source_t prof_src)
{
    this->out_dir = out_dir;
    this->symbol_map = symbol_map;
    this->perf_event = perf_event;
    this->prof_src = prof_src;
    // set up symbol tracking
    symbol_lut.resize(symbol_map.size() + 1); // 0th index is reserved
    for (const auto &s : symbol_map) {
        symbol_lut[s.second.idx] = {s.first, s.second.name};
    }
    // set up beginning of callstack
    st.idx_callstack.push_back(symbol_map.at(BASE_ADDR).idx);
    st.idx_callstack_prev = st.idx_callstack;
    set_fallthrough_symbol(BASE_ADDR);
    st.updated = false;
    callstack_cnt = 0;
}

bool profiler_perf::finish_inst(uint32_t next_pc) {
    // handle next instruction symbol change, if any
    bool fallthrough = ((next_pc == st.fallthrough_pc) && !st.updated);
    if (fallthrough) {
        update_callstack(next_pc);
        st.idx_callstack.back() = symbol_map.at(next_pc).idx;
    }

    if (!match_top(next_pc)) {
        diverged_cnt += 1;
        // try to fix diverged callstack

        //std::cout << "diverged at next_pc: " << MEM_ADDR_FORMAT(next_pc)
        //          << std::flush;

        // tail calls can be missed, try to pop back and match the current top
        auto temp_cs = st.idx_callstack;
        while (!match_top(next_pc) && !temp_cs.empty()) temp_cs.pop_back();

        // if pop_back resolves the diverged callstack, use it
        // else, symbol change likely due to assembly labels, just replace top
        if (!temp_cs.empty()) {
            st.idx_callstack = temp_cs;
        } else {
            auto found_sym = find_symbol_in_range(next_pc);
            st.idx_callstack.back() = symbol_map.at(found_sym.first).idx;
        }

        //if (temp_cs.empty()) {
        //    std::cout << "    was empty\n" << std::flush;
        //}
    }

    // handle counter
    if (st.updated) callstack_cnt = 0;
    else inc_callstack_cnt();
    st.updated = false;

    // ret
    if (st.idx_callstack != st.idx_callstack_prev) {
        st.idx_callstack_prev = st.idx_callstack;
        return true;
    }
    return false;
}

void profiler_perf::update_branch(uint32_t next_pc, bool taken) {
    if (!taken) return;
    auto found_sym = find_symbol_in_range(next_pc);
    bool sym_chg = (found_sym.second.idx != st.idx_callstack.back());
    if (sym_chg) {
        update_callstack(found_sym.first);
        st.idx_callstack.back() = symbol_map.at(found_sym.first).idx;
    }
}

void profiler_perf::update_jalr(
    uint32_t next_pc, bool ret_inst, bool tail_call, uint32_t ra) {
    // also not ret if it doesn't change the symbol
    ret_inst &= symbol_change_on_jump(next_pc);
    if (ret_inst) {
        update_callstack(next_pc);
        // catch_empty_callstack("jalr (ret)", next_pc);
        st.idx_callstack.pop_back();
        if (symbol_map.find(next_pc) != symbol_map.end()) {
            // returns to the next symbol (mostly-assembly thing)
            if (st.idx_callstack.empty()) {
                st.idx_callstack.push_back(symbol_map.at(next_pc).idx);
            } else {
                st.idx_callstack.back() = symbol_map.at(next_pc).idx;
            }
        }
    } else if (symbol_map.find(next_pc) != symbol_map.end()) {
        update_callstack(next_pc);
        bool noreturn_call = symbol_map.find(ra) != symbol_map.end();
        if (tail_call || noreturn_call) {
            // catch_empty_callstack("jalr", next_pc);
            st.idx_callstack.pop_back();
        }
        st.idx_callstack.push_back(symbol_map.at(next_pc).idx);
    }
}

void profiler_perf::update_jal(uint32_t next_pc, bool tail_call, uint32_t ra) {
    bool noreturn_call = symbol_map.find(ra) != symbol_map.end();
    if (symbol_map.find(next_pc) != symbol_map.end()) {
        update_callstack(next_pc);
        if (tail_call || noreturn_call) {
            st.idx_callstack.pop_back();
            // catch_empty_callstack("jal", next_pc);
        }
        st.idx_callstack.push_back(symbol_map.at(next_pc).idx);
    }
}

void profiler_perf::inc_callstack_cnt() {
    if (!active) {
        perf_event_flags[TO_U32(perf_event)] = 0;
        return;
    }
    if (perf_event == perf_event_t::inst) {
        callstack_cnt += 1;
    #ifdef DPI
    } else if (perf_event == perf_event_t::cycle) {
        callstack_cnt += clk_src->get_diff();
    #endif
    } else {
        callstack_cnt += perf_event_flags[TO_U32(perf_event)];
        // only care about resetting the event being counted, others dc
        perf_event_flags[TO_U32(perf_event)] = 0;
    }
}

void profiler_perf::save_callstack_cnt() {
    callstack_cnt_map[callstack_to_key()] += callstack_cnt;
}

void profiler_perf::catch_empty_callstack(
    const std::string& inst, uint32_t next_pc) {
    if (st.idx_callstack.empty()) {
        std::cerr << "ERROR: " << inst << ": callstack underflow at "
                  << std::hex << next_pc << std::dec << std::endl;
        throw std::runtime_error("callstack underflow");
    }
}

void profiler_perf::update_callstack(uint32_t next_pc) {
    // finish the current callstack
    inc_callstack_cnt();
    save_callstack_cnt();
    // start new callstack
    set_fallthrough_symbol(next_pc);
    st.updated = true;
}

void profiler_perf::set_fallthrough_symbol(uint32_t pc) {
    //bool found = false;
    for (auto it = symbol_map.begin(); it != symbol_map.end(); it++) {
        if (it->first == pc) {
            //found = true;
            it++;
            if (it == symbol_map.end()) break; // no next symbol
            st.fallthrough_pc = it->first;
        }
    }
    //if (!found) {
    //    std::cerr << "ERROR: set_fallthrough_symbol: symbol at pc "
    //              << std::hex << pc << " not found" << std::dec << std::endl;
    //    throw std::runtime_error("set_fallthrough_symbol: symbol not found");
    //}
}

bool profiler_perf::symbol_change_on_jump(uint32_t next_pc) {
    auto found_sym = find_symbol_in_range(next_pc);
    return (found_sym.second.idx != st.idx_callstack.back());
}

std::pair<uint32_t, symbol_map_entry_t>
    profiler_perf::find_symbol_in_range(uint32_t next_pc) {
    // find to which range does the next_pc belong
    std::pair<uint32_t, symbol_map_entry_t> found;
    for (const auto &sym : symbol_map) {
        if (next_pc >= sym.first) found = sym;
        else break;
    }
    return found;
}

std::string profiler_perf::get_callstack_str(
    const std::vector<uint16_t>& idx_stack) {
    std::string stack_str = "";
    for (const auto &idx : idx_stack) {
        stack_str += symbol_lut.at(idx).name + ";";
    }
    return stack_str;
}

std::u16string profiler_perf::callstack_to_key() {
  auto p = reinterpret_cast<const char16_t*>(st.idx_callstack.data());
  return std::u16string(p, st.idx_callstack.size());
}

void profiler_perf::log_to_file_and_print(bool silent) {
    // close last callstack
    save_callstack_cnt();
    std::string pt = "";
    if (prof_src == profiler_source_t::clock) pt = "clk_";

    // dump all counters from callstack_cnt_map to file
    std::string out = out_dir + "callstack_folded_" + pt +
                      perf_event_names[TO_U32(perf_event)] + ".txt";
    std::ofstream out_file(out);
    std::vector<uint16_t> callstack_ids;
    uint64_t total_cnt = 0;
    for (const auto &c : callstack_cnt_map) {
        callstack_ids.clear();
        callstack_ids.reserve(c.first.size());
        // demangle the key string
        for (const auto &id : c.first) callstack_ids.push_back(TO_U16(id));
        // write to file
        out_file << get_callstack_str(callstack_ids) << " " << c.second << "\n";
        total_cnt += c.second;
    }

    #ifdef DPI
    return;
    #endif

    if (silent) return;

    std::cout << "Profiler - Perf:\n"
              << INDENT << "Event: " << perf_event_names[TO_U32(perf_event)]
              << ", Samples: " << total_cnt << "\n";
    if (diverged_cnt) {
            std::cout << INDENT << "Warning: Stacktop divergence detected "
                      << diverged_cnt << " times\n";
    }
}

bool profiler_perf::match_top(uint32_t next_pc) {
    uint16_t idx = 0;
    for (const auto &sym : symbol_map) {
        if (next_pc >= sym.first) idx = sym.second.idx;
        else break;
    }
    uint16_t top = {st.idx_callstack.back()};
    return (idx == top);
}

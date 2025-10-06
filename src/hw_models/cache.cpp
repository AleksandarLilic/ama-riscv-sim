#include "cache.h"
#include "main_memory.h"

cache::cache(
    cache_type_t type,
    uint32_t sets,
    uint32_t ways,
    cache_re_policy_t re_policy,
    cache_wr_policy_t wr_policy,
    std::string cache_name) :
        type(type),
        sets(sets),
        ways(ways),
        re_policy(re_policy),
        wr_policy(wr_policy),
        cache_name(cache_name)
    {
    validate_inputs(sets, ways, re_policy, wr_policy);
    direct_mapped = (ways == 1);

    // first dim is number of sets
    cache_entries.resize(sets);
    // second dim number of ways in a set
    // go through all entires and initialize lru_cnt and data
    for (uint32_t set = 0; set < sets; set++) {
        cache_entries[set].resize(ways);
        for (uint32_t way = 0; way < ways; way++) {
            auto& line = cache_entries[set][way];
            line.metadata.lru_cnt = way;
            #if CACHE_MODE == CACHE_MODE_FUNC
            std::fill(line.data.begin(), line.data.end(), 0xCD);
            #endif
        }
    }

    index_bits_num = 0;
    index_mask = 0;
    for (uint32_t i = sets; i > 1; i >>= 1) {
        index_bits_num++;
        index_mask = (index_mask << 1) | 1;
    }
    tag_bits_num = ADDR_BITS - index_bits_num - CACHE_BYTE_ADDR_BITS;
    tag_off = (CACHE_BYTE_ADDR_BITS + index_bits_num);
    max_scp_ways = ways - 1; // should be fw configurable as MMIO, max = ways-1
    metadata_bits_num = metadata_t::get_bits_num();
    metadata_bits_num += log2(ways); // lru counters
    size = {sets, ways, CACHE_LINE_SIZE, tag_bits_num, metadata_bits_num};
}

uint32_t cache::rd(uint32_t addr, uint32_t size) {
    auto ret = reference(addr, size, mem_op_t::read, scp_mode_t::m_none);
    if (ret == cache_ref_t::miss) {
        miss(addr, size, mem_op_t::read, scp_mode_t::m_none);
    }
    return rd_buf;
}

void cache::wr(uint32_t addr, uint32_t data, uint32_t size) {
    wr_buf = data;
    auto ret = reference(addr, size, mem_op_t::write, scp_mode_t::m_none);
    if (ret == cache_ref_t::miss) {
        miss(addr, size, mem_op_t::write, scp_mode_t::m_none);
    }
}

scp_status_t cache::scp_lcl(uint32_t addr) {
    if (direct_mapped) {
        SIM_WARNING << "Cache '" << cache_name
                    << "' is direct-mapped but tried to create SCP line. SCP "
                       "needs at least 2-way set-associative cache\n";
    }
    auto ret = reference(addr, 0u, mem_op_t::read, scp_mode_t::m_lcl);
    if (ret == cache_ref_t::miss) {
        miss(addr, 0u, mem_op_t::read, scp_mode_t::m_lcl);
    }
    return scp_status;
}

scp_status_t cache::scp_rel(uint32_t addr) {
    if (direct_mapped) {
        SIM_WARNING << "Cache '" << cache_name
                    << "' is direct-mapped but tried to release SCP line. SCP "
                       "needs at least 2-way set-associative cache\n";
    }
    auto ret = reference(addr, 0u, mem_op_t::read, scp_mode_t::m_rel);
    if (ret == cache_ref_t::miss) {
        miss(addr, 0u, mem_op_t::read, scp_mode_t::m_rel);
    }
    return scp_status;
}

void cache::speculative_exec(speculative_t smode) {
    this->smode = smode;
    speculative_exec_active = (smode == speculative_t::enter);
}

// uses real address so that the tags are appropriately set
cache_ref_t cache::reference(
    uint32_t addr, uint32_t size, mem_op_t atype, scp_mode_t scp_mode) {
    // don't count reference for scp release, no data is referenced
    if (scp_mode != scp_mode_t::m_rel) {
        stats.referenced(atype, size);
        if (roi.has(addr)) roi.stats.referenced(atype, size);
        #ifdef PROFILERS_EN
        prof_perf->set_perf_event_flag(ref_event);
        #endif
    }
    ccl.init(
        (addr>>CACHE_BYTE_ADDR_BITS) & index_mask,
        addr>>tag_off,
        {0, 0},
        addr & CACHE_BYTE_ADDR_MASK
    );

    for (uint32_t way = 0; way < ways; way++) {
        auto& line = cache_entries[ccl.index][way];
        if (line.metadata.valid && (line.tag == ccl.tag)) {
            // hit, doesn't go to main mem
            scp_status = update_scp(scp_mode, line, ccl.index);

            #ifdef DASM_EN
            hwmi_ptr->log_cache({
                cache_name,
                addr,
                ccl.tag,
                ccl.index,
                way,
                ccl.byte_addr,
                atype,
                (line.metadata.scp == true),
                true
            });
            #endif

            // don't update lru on release
            if (scp_mode == scp_mode_t::m_rel) {
                *hws = hw_status_t::hit;
                return cache_ref_t::hit;
            }

            update_lru(ccl.index, way);
            line.referenced();
            stats.hit(atype);
            if (roi.has(addr)) roi.stats.hit(atype);

            if (atype == mem_op_t::write) {
                #if CACHE_MODE == CACHE_MODE_FUNC
                write_to_cache(ccl.byte_addr, size, line);
                #endif
                if (wr_policy == cache_wr_policy_t::wt) {
                    stats.writeback();
                    if (roi.has(line.tag << tag_off)) roi.stats.writeback();
                } else {
                    line.metadata.dirty = true;
                }
            } else { // read
                #if CACHE_MODE == CACHE_MODE_FUNC
                read_from_cache(ccl.byte_addr, size, line);
                #endif
            }

            *hws = hw_status_t::hit;
            return cache_ref_t::hit;

        } else { // keep looking for victim line
            // potentially a miss, keep the largest lru count as victim line
            if ((line.metadata.lru_cnt >= ccl.victim.lru_cnt) &&
                !line.metadata.scp) {
                // if there are ways-1 scp lines in the set, it can happen
                // for lru 0 to be victim, hence the >= instead of > comparison
                // as ccl is initialized to 0
                ccl.victim = {way, line.metadata.lru_cnt};
            }
        }
    }

    if (scp_mode == scp_mode_t::m_rel) {
        // can't release on miss, assume to be an erroneous attempt from SW
        // e.g. lcl attempts in direct-mapped caches would always fail
        // so there would be nothing to release, yet SW may try
        SIM_WARNING << "Cache '" << cache_name
                    << "' tried to release SCP line at address: 0x"
                    << MEM_ADDR_FORMAT(addr)
                    << " but cache missed, nothing has been released\n";
        return cache_ref_t::ignore;
    }
    *hws = hw_status_t::miss;
    return cache_ref_t::miss;
}

void cache::miss(
    uint32_t addr,
    [[maybe_unused]] uint32_t size,
    mem_op_t atype,
    scp_mode_t scp_mode) {
    // TODO: don't service misses in speculative mode?

    // miss, goes to main mem
    stats.miss(atype);
    if (roi.has(addr)) roi.stats.miss(atype);

    #ifdef PROFILERS_EN
    prof_perf->set_perf_event_flag(miss_event);
    #endif

    #ifdef DASM_EN
    hwmi_ptr->log_cache({
        cache_name,
        addr,
        ccl.tag,
        ccl.index,
        ccl.victim.way_idx,
        ccl.byte_addr,
        atype,
        (scp_mode == scp_mode_t::m_lcl),
        false
    });
    #endif

    // replace the line (evict if dirty)
    auto& act_line = cache_entries[ccl.index][ccl.victim.way_idx];
    if (act_line.metadata.valid) {
        stats.replace(act_line.metadata.dirty);
        if (roi.has(act_line.tag << tag_off)) {
            roi.stats.replace(act_line.metadata.dirty);
        }
        if (act_line.metadata.dirty) {
            #if CACHE_MODE == CACHE_MODE_FUNC
            mem->wr_line((act_line.tag << tag_off) - BASE_ADDR, act_line.data);
            #endif
            act_line.metadata.dirty = false;
        }
    }

    // bring in the new line requested by the core
    act_line.referenced();
    act_line.tag = ccl.tag; // act_line now caching new data
    act_line.metadata.valid = true;
    update_lru(ccl.index, ccl.victim.way_idx); // now the most recently used
    scp_status = update_scp(scp_mode, act_line, ccl.index);

    #if CACHE_MODE == CACHE_MODE_FUNC
    act_line.data = mem->rd_line(addr - BASE_ADDR);
    #endif
    if (atype == mem_op_t::write) {
        #if CACHE_MODE == CACHE_MODE_FUNC
        write_to_cache(ccl.byte_addr, size, act_line);
        #endif
        if (wr_policy == cache_wr_policy_t::wt) {
            stats.writeback();
            if (roi.has(act_line.tag << tag_off)) roi.stats.writeback();
        } else {
            act_line.metadata.dirty = true;
        }
    } else { // read
        #if CACHE_MODE == CACHE_MODE_FUNC
        read_from_cache(ccl.byte_addr, size, act_line);
        #endif
    }


    #ifdef SCP_BACKDOOR
    // useful for debugging, to convert a specific address to scratchpad
    // NOTE: doesn't handle too many scp requests in this #ifdef
    if (act_line.metadata.scp == false) {
        if ((act_line.tag == TO_U32(0x17200>>CACHE_BYTE_ADDR_BITS)) ||
            (act_line.tag == TO_U32((0x17200 + 64)>>CACHE_BYTE_ADDR_BITS)) ||
            (act_line.tag == TO_U32((0x17200 + 128)>>CACHE_BYTE_ADDR_BITS)) ||
            (act_line.tag == TO_U32((0x17200 + 192)>>CACHE_BYTE_ADDR_BITS))) {
            act_line.metadata.scp = true; // converted to scratchpad
            std::cout << "Converted to scratchpad: " << std::hex << addr <<"\n";
        }
    }
    // release all if the roi is done
    if (roi.stats.references >= 4096) {
        for (uint32_t set = 0; set < sets; set++) {
            for (uint32_t way = 0; way < ways; way++) {
                if (cache_entries[set][way].metadata.scp) {
                    std::cout << "Releasing: " << std::hex
                              << cache_entries[set][way].tag << "\n";
                    cache_entries[set][way].metadata.scp = false;
                }
            }
        }
    }
    #endif

    // TODO: prefetcher
    return;
}

void cache::update_lru(uint32_t index, uint32_t way) {
    // TODO: don't update lru in speculative mode?
    //if (smode == speculative_t::enter) return;
    auto& active_set = cache_entries[index];
    auto& active_lru_cnt = active_set[way].metadata.lru_cnt;
    // increment all counters newer than the current way
    for (uint32_t i = 0; i < ways; i++) {
        auto& line = active_set[i];
        if (line.metadata.lru_cnt < active_lru_cnt) line.metadata.lru_cnt++;
    }
    // reset the current way
    active_lru_cnt = 0;
}

scp_status_t cache::update_scp(
    scp_mode_t scp_mode, cache_line_t& line,uint32_t index) {
    if (scp_mode == scp_mode_t::m_none) return scp_status_t::success;
    if (scp_mode == scp_mode_t::m_lcl) return convert_to_scp(line, index);
    else if (scp_mode == scp_mode_t::m_rel) return release_scp(line);
    // TODO: exception code 24: custom use - unknown scp mode
    // else tu.e_hardware_error("Unknown SCP mode");
    else throw std::runtime_error("Unknown SCP mode.");
}

scp_status_t cache::convert_to_scp(cache_line_t& line, uint32_t index) {
    // first check if there are empty ways for conversion
    uint32_t scp_cnt = 0;
    for (uint32_t way = 0; way < ways; way++) {
        if (cache_entries[index][way].metadata.scp) scp_cnt++;
    }
    if (scp_cnt < max_scp_ways) {
        line.metadata.scp = true;
        return scp_status_t::success;
    } else {
        return scp_status_t::fail;
    }
}

scp_status_t cache::release_scp(cache_line_t& line) {
    if (line.metadata.scp) {
        line.metadata.scp = false;
        return scp_status_t::success;
    } else {
        return scp_status_t::fail;
    }
}

void cache::validate_inputs(
    uint32_t sets,
    uint32_t ways,
    cache_re_policy_t re_policy,
    cache_wr_policy_t wr_policy
) {
    bool error = false;

    if (re_policy != cache_re_policy_t::lru) {
        std::cerr << "ERROR: " << cache_name
                  << ": only LRU re_policy is supported" << std::endl;
        error = true;
    }

    if (type == cache_type_t::inst &&
        wr_policy != cache_wr_policy_t::none) {
        std::cerr << "ERROR: " << cache_name
                  << ": instruction cache cannot have a write policy"
                  << std::endl;
        error = true;
    } else if (type == cache_type_t::data &&
               wr_policy != cache_wr_policy_t::wb &&
               wr_policy != cache_wr_policy_t::wt) {
        std::cerr << "ERROR: " << cache_name
                  << ": only write-back and write-through policies are "
                     "supported"
                  << std::endl;
        error = true;
    }

    if (sets == 0) {
        std::cerr << "ERROR: " << cache_name
                  << ": number of sets cannot be 0" << std::endl;
        error = true;
    }

    if (sets > MAX_CACHE_SETS) {
        std::cerr << "ERROR: " << cache_name
                  << ": number of sets cannot exceed " << MAX_CACHE_SETS
                  << ". Specified: " << sets << std::endl;
        error = true;
    }

    if (!is_pow_2(sets)) {
        std::cerr << "ERROR: " << cache_name
                  << ": number of sets must be a power of 2. Specified: "
                  << sets << std::endl;

        error = true;
    }

    if (ways == 0) {
        std::cerr << "ERROR: " << cache_name
                  << ": number of ways cannot be 0" << std::endl;
        error = true;
    }

    if (ways > MAX_CACHE_WAYS) {
        std::cerr << "ERROR: " << cache_name
                  << ": number of ways cannot exceed " << MAX_CACHE_WAYS
                  << ". Specified: " << ways << std::endl;
        error = true;
    }

    if (error) throw std::runtime_error("Invalid cache inputs encountered");
}

#if CACHE_MODE == CACHE_MODE_FUNC
void cache::read_from_cache(
    uint32_t byte_addr, uint32_t size, cache_line_t& act_line) {
    rd_buf = 0;
    for (uint32_t i = 0; i < size; i++) {
        rd_buf |= act_line.data[byte_addr + i] << (i * 8);
    }
}

void cache::write_to_cache(
    uint32_t byte_addr, uint32_t size, cache_line_t& act_line) {
    for (uint32_t i = 0; i < size; i++) {
        act_line.data[byte_addr + i] = TO_U8(wr_buf >> (i * 8));
    }
    if (wr_policy == cache_wr_policy_t::wt) {
        mem->wr_line((act_line.tag << tag_off) - BASE_ADDR, act_line.data);
    }
}
#endif

// stats
void cache::set_roi(uint32_t start, uint32_t size) { roi.set(start, size); }

void cache::show_stats(bool show_state) {
    std::cout << cache_name;
    size.show();
    std::cout << "\n" << INDENT;
    stats.show();
    std::cout << "\n";

    if (show_state) {
        // find n as a largest number of digits - for alignment in stdout
        int32_t n = 0;
        for (uint32_t set = 0; set < sets; set++) {
            for (uint32_t way = 0; way < ways; way++) {
                uint32_t cnt = cache_entries[set][way].reference_cnt;
                n = std::max(n, TO_I32(std::to_string(cnt).size()));
            }
        }

        for (uint32_t set = 0; set < sets; set++) {
            std::cout << INDENT << "s" << std::left << std::setw(2) << set
                      << ": " << std::right;
            for (uint32_t way = 0; way < ways; way++) {
                std::cout << " w" << way << " [" << std::setw(n)
                        << cache_entries[set][way].reference_cnt << "] ";
            }
            std::cout << "\n";
        }
    }

    if (!(roi.start == 0 && roi.end == 0)) {
        std::cout << INDENT << "ROI: "
                  << "(0x" << std::hex << roi.start
                  << " - 0x" << roi.end << "): " << std::dec;
        roi.stats.show();
        std::cout << "\n";
    }
    // dump();
}

void cache::log_stats(std::ofstream& hw_ofs) {
    hw_ofs << "\"" << cache_name << "\"" << ": {";
    stats.log(hw_ofs);
    hw_ofs << ", ";
    size.log(hw_ofs);
    hw_ofs << "\n}," << std::endl;
}

void cache::dump() const {
    std::cout << "  state:" << "\n";
    for (uint32_t set = 0; set < sets; set++) {
        for (uint32_t way = 0; way < ways; way++) {
            auto& line = cache_entries[set][way];
            std::cout << "    s" << set << " w" << way
                      << ", tag: " << FHEXZ(line.tag, 4)
                      << ", lru: " << line.metadata.lru_cnt
                      << ", scp: " << line.metadata.scp
                      << ", valid: " << line.metadata.valid
                      << ", dirty: " << line.metadata.dirty
                      << ", reference_cnt: " << line.reference_cnt
                      << "\n";
            #if CACHE_MODE == CACHE_MODE_FUNC
            // dump data in the line, byte by byte, all 64 bytes in a line
            std::cout << "     ";
            for (uint32_t i = 0; i < CACHE_LINE_SIZE; i++) {
                std::cout << " " << std::hex << std::setw(2)
                          << std::setfill('0') << TO_U32(line.data[i]);
                if (i % 4 == 3) std::cout << " ";
                if (i % 64 == 63) std::cout << "\n";
            }
            #endif
        }
    }
    std::cout << std::dec << "\n";
}

#include "cache.h"
#include "main_memory.h"

cache::cache(uint32_t sets, uint32_t ways,
             std::string cache_name, main_memory* mem)
    : sets(sets), ways(ways), cache_name(cache_name), mem(mem) {
    if (!is_pow_2(sets)) {
        std::string err_msg =
            "Number of sets (in: " + std::to_string(sets) +
            ") must be a power of 2.";
        throw std::runtime_error(err_msg);
    }

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
    // half of the ways in a any set can be converted to scratchpad, at most
    max_scp_ways = ways >> 1;
    metadata_bits_num = metadata_t::get_bits_num();
    metadata_bits_num += log2(ways); // lru counters
    size = {sets, ways, CACHE_LINE_SIZE, tag_bits_num, metadata_bits_num};
}

uint32_t cache::rd(uint32_t addr, uint32_t size) {
    access(addr, size, access_t::read, scp_mode_t::m_none);
    return rd_buf;
}

scp_status_t cache::scp_lcl(uint32_t addr) {
    access(addr, 0u, access_t::read, scp_mode_t::m_lcl);
    return scp_status;
}

scp_status_t cache::scp_rel(uint32_t addr) {
    access(addr, 0u, access_t::read, scp_mode_t::m_rel);
    return scp_status;
}

void cache::wr(uint32_t addr, uint32_t data, uint32_t size) {
    wr_buf = data;
    access(addr, size, access_t::write, scp_mode_t::m_none);
}

void cache::speculative_exec(speculative_t smode) {
    this->smode = smode;
    speculative_exec_active = (smode == speculative_t::enter);
}

// uses the real address so that the tags are appropriately set
void cache::access(uint32_t addr, uint32_t size,
                   access_t atype, scp_mode_t scp_mode) {
    // don't count access for scp release, no data is accessed
    if (scp_mode != scp_mode_t::m_rel) {
        stats.access(atype, size);
        if (roi.has(addr)) roi.stats.access(atype, size);
    }
    #if CACHE_MODE == CACHE_MODE_FUNC
    uint32_t byte_addr = addr & CACHE_BYTE_ADDR_MASK;
    #endif
    uint32_t index = (addr >> CACHE_BYTE_ADDR_BITS) & index_mask;
    uint32_t tag = addr >> tag_off;
    target_t target;

    for (uint32_t way = 0; way < ways; way++) {
        auto& line = cache_entries[index][way];
        if (line.metadata.valid && line.tag == tag) {
            // hit, doesn't go to main mem
            scp_status = update_scp(scp_mode, line, index);
            // don't update lru on release
            if (scp_mode == scp_mode_t::m_rel) return;
            update_lru(index, way);
            line.access();
            stats.hit();
            if (roi.has(addr)) roi.stats.hit();
            #if CACHE_MODE == CACHE_MODE_FUNC
            if (atype == access_t::write) write_to_cache(byte_addr, size, line);
            else read_from_cache(byte_addr, size, line);
            #else
            if (atype == access_t::write) line.metadata.dirty = true;
            #endif
            return;

        } else {
            if (line.metadata.lru_cnt > target.lru_cnt && !line.metadata.scp) {
                target = {way, line.metadata.lru_cnt};
            }
        }
    }

    // can't release on miss, assume to be an erroneous attempt from SW
    if (scp_mode == scp_mode_t::m_rel) return;

    // TODO: don't service misses in speculative mode?

    // miss, goes to main mem
    stats.miss();
    if (roi.has(addr)) roi.stats.miss();

    // evict the line
    auto& act_line = cache_entries[index][target.way_idx]; // line being evicted
    if (act_line.metadata.valid) {
        stats.evict(act_line.metadata.dirty);
        if (roi.has(act_line.tag << tag_off)) {
            roi.stats.evict(act_line.metadata.dirty);
        }
        if (act_line.metadata.dirty) {
            #if CACHE_MODE == CACHE_MODE_FUNC and defined(CACHE_VERIFY)
            mem->wr_line((act_line.tag << tag_off) - BASE_ADDR, act_line.data);
            #endif
            act_line.metadata.dirty = false;
        }
    }

    // bring in the new line requested by the core
    act_line.access();
    act_line.tag = tag; // act_line now caching new data
    act_line.metadata.valid = true;
    update_lru(index, target.way_idx); // now the most recently used
    scp_status = update_scp(scp_mode, act_line, index);
    #if CACHE_MODE == CACHE_MODE_FUNC
    act_line.data = mem->rd_line(addr - BASE_ADDR);
    if (atype == access_t::write) write_to_cache(byte_addr, size, act_line);
    else read_from_cache(byte_addr, size, act_line);
    #else
    if (atype == access_t::write) act_line.metadata.dirty = true;
    #endif

    #ifdef SCP_BACKDOOR
    // useful for debugging, to convert a specific address to scratchpad
    // NOTE: doesn't handle too many scp requests in this #ifdef
    if (act_line.metadata.scp == false) {
        if ((act_line.tag == TO_U32(0x17200>>CACHE_BYTE_ADDR_BITS)) ||
            (act_line.tag == TO_U32((0x17200 + 64)>>CACHE_BYTE_ADDR_BITS)) ||
            (act_line.tag == TO_U32((0x17200 + 128)>>CACHE_BYTE_ADDR_BITS)) ||
            (act_line.tag == TO_U32((0x17200 + 192)>>CACHE_BYTE_ADDR_BITS)))
            {
            act_line.metadata.scp = true; // converted to scratchpad
            std::cout << "Converted to scratchpad: " << std::hex << addr
                      << std::endl;
        }
    }
    // release all if the roi is done
    if (roi.stats.accesses >= 4096) {
        for (uint32_t set = 0; set < sets; set++) {
            for (uint32_t way = 0; way < ways; way++) {
                if (cache_entries[set][way].metadata.scp) {
                    std::cout << "Releasing: " << std::hex
                              << cache_entries[set][way].tag << std::endl;
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

scp_status_t cache::update_scp(scp_mode_t scp_mode, cache_line_t& line,
                           uint32_t index){
    if (scp_mode == scp_mode_t::m_none) return scp_status_t::success;
    if (scp_mode == scp_mode_t::m_lcl) return convert_to_scp(line, index);
    else if (scp_mode == scp_mode_t::m_rel) return release_scp(line);
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

// TODO: release all scp entries

#if CACHE_MODE == CACHE_MODE_FUNC
void cache::read_from_cache(uint32_t byte_addr, uint32_t size,
                            cache_line_t& act_line) {
    rd_buf = 0;
    for (uint32_t i = 0; i < size; i++)
        rd_buf |= act_line.data[byte_addr + i] << (i * 8);
}

void cache::write_to_cache(uint32_t byte_addr, uint32_t size,
                           cache_line_t& act_line) {
    // write-back policy, just mark the line as dirty
    act_line.metadata.dirty = true;
    for (uint32_t i = 0; i < size; i++)
        act_line.data[byte_addr + i] = TO_U8(wr_buf >> (i * 8));
}
#endif

// stats
void cache::set_roi(uint32_t start, uint32_t size) { roi.set(start, size); }

void cache::show_stats() {
    std::cout << cache_name;
    size.show();
    stats.show();
    std::cout << std::endl;

    // find n as a largest number of digits - for alignment in stdout
    int32_t n = 0;
    for (uint32_t set = 0; set < sets; set++) {
        for (uint32_t way = 0; way < ways; way++) {
            uint32_t cnt = cache_entries[set][way].access_cnt;
            n = std::max(n, TO_I32(std::to_string(cnt).size()));
        }
    }

    for (uint32_t set = 0; set < sets; set++) {
        std::cout << INDENT << "s" << set << ": ";
        for (uint32_t way = 0; way < ways; way++) {
            std::cout << " w" << way << " [" << std::setw(n)
                      << cache_entries[set][way].access_cnt << "] ";
        }
        std::cout << std::endl;
    }

    if (!(roi.start == 0 && roi.end == 0)) {
        std::cout << INDENT << "ROI: "
                  << "(0x" << std::hex << roi.start
                  << " - 0x" << roi.end << "): " << std::dec;
        roi.stats.show();
        std::cout << std::endl;
    }
    // dump();
}

void cache::log_stats(std::ofstream& log_file) {
    log_file << "\"" << cache_name << "\"" << ": {";
    stats.log(log_file);
    log_file << ", ";
    size.log(log_file);
    log_file << "}," << std::endl;
}

void cache::dump() const {
    std::cout << "  state:" << std::endl;
    for (uint32_t set = 0; set < sets; set++) {
        for (uint32_t way = 0; way < ways; way++) {
            auto& line = cache_entries[set][way];
            std::cout << "    s" << set << " w" << way
                      << " tag: 0x" << std::hex << line.tag
                      << " lru: " << std::dec << line.metadata.lru_cnt
                      << " scp: " << line.metadata.scp
                      << " valid: " << line.metadata.valid
                      << " dirty: " << line.metadata.dirty
                      << " access_cnt: " << line.access_cnt
                      << std::endl;
            #if CACHE_MODE == CACHE_MODE_FUNC
            // dump data in the line, byte by byte, all 64 bytes in a line
            std::cout << "     ";
            for (uint32_t i = 0; i < CACHE_LINE_SIZE; i++) {
                std::cout << " " << std::hex << std::setw(2)
                          << std::setfill('0') << (uint32_t)line.data[i];
                if (i % 4 == 3) std::cout << " ";
                if (i % 64 == 63) std::cout << std::endl;
            }
            #endif
        }
    }
    std::cout << std::dec << std::endl;
}

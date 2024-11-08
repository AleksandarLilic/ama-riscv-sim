#include "cache.h"

cache::cache(uint32_t sets, uint32_t ways, std::string cache_name)
    : sets(sets), ways(ways), cache_name(cache_name) {
    if (!is_pow_2(sets) || !is_pow_2(ways)) {
        std::string err_msg =
            "Number of sets (in: " + std::to_string(sets) +
            ") and ways (in: " + std::to_string(ways) +
            ") must be a power of 2.";
        throw std::runtime_error(err_msg);
    }

    // first dim is number of sets
    cache_entries.resize(sets);
    // second dim number of ways in a set
    // go through all entires and initialize lru_cnt
    for (uint32_t set = 0; set < sets; set++) {
        cache_entries[set].resize(ways);
        for (uint32_t way = 0; way < ways; way++)
            cache_entries[set][way].metadata.lru_cnt = way;
    }

    index_bits_num = 0;
    index_mask = 0;
    for (uint32_t i = sets; i > 1; i >>= 1) {
        index_bits_num++;
        index_mask = (index_mask << 1) | 1;
    }
    tag_bits_num = 32 - index_bits_num - CACHE_BYTE_ADDR_BITS;
}

void cache::set_roi(uint32_t start, uint32_t end) { roi.set(start, end); }

void cache::update_lru(uint32_t index, uint32_t way) {
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

void cache::access(uint32_t addr, uint32_t size, access_type_t atype) {
    stats.accesses++;
    if (roi.has(addr)) roi.stats.accesses++;
    uint32_t tag_off = (CACHE_BYTE_ADDR_BITS + index_bits_num);
    //uint32_t byte_addr = addr & CACHE_BYTE_ADDR_MASK;
    uint32_t index = (addr >> CACHE_BYTE_ADDR_BITS) & index_mask;
    uint32_t tag = addr >> tag_off;
    evict_t evict;

    for (uint32_t way = 0; way < ways; way++) {
        auto& line = cache_entries[index][way];
        if (line.metadata.valid && line.tag == tag) {
            // hit, doesn't go to main mem
            line.access_cnt++;
            stats.hit(atype, size);
            if (roi.has(addr)) roi.stats.hit(atype, size);
            // write-back policy, just mark the line as dirty
            if (atype == access_type_t::write) line.metadata.dirty = true;
            update_lru(index, way);
            return;

        } else {
            if (line.metadata.lru_cnt > evict.lru_cnt) {
                evict.lru_cnt = line.metadata.lru_cnt;
                evict.way = way;
            }
        }
    }

    // miss, goes to main mem
    stats.miss();
    if (roi.has(addr)) roi.stats.miss();

    // evict the line
    auto& act_line = cache_entries[index][evict.way]; // line being evicted
    act_line.access_cnt++;
    if (act_line.metadata.valid) {
        stats.evicts++;
        if (roi.has(act_line.tag << tag_off)) roi.stats.evicts++;
        if (act_line.metadata.dirty) {
            //mem->wr_line(addr, line.data);
            act_line.metadata.dirty = false;
            stats.writeback();
            if (roi.has(act_line.tag << tag_off)) roi.stats.writeback();
        }
    }

    // bring in the new line requested by the core
    //line.data = mem->rd_line(addr);
    act_line.tag = tag; // same line, now caching new data
    act_line.metadata.valid = true;
    update_lru(index, evict.way);
    if (atype == access_type_t::write) {
        act_line.metadata.dirty = true;
        stats.bw_core.writes += size;
        if (roi.has(addr)) roi.stats.bw_core.writes += size;
    } else {
        stats.bw_core.reads += size;
        if (roi.has(addr)) roi.stats.bw_core.reads += size;
    }

    // TODO: prefetcher

    return;
}

// TODO: reads should return in 'func' mode
uint8_t cache::rd8(uint32_t addr) {
    access(addr, 1, access_type_t::read);
    return 0;
}

uint16_t cache::rd16(uint32_t addr) {
    access(addr, 2, access_type_t::read);
    return 0;
}

uint32_t cache::rd32(uint32_t addr) {
    access(addr, 4, access_type_t::read);
    return 0;
}

// TODO: writes should take in data in 'func' mode and write to cache
void cache::wr8(uint32_t addr) { access(addr, 1, access_type_t::write); }
void cache::wr16(uint32_t addr) { access(addr, 2, access_type_t::write); }
void cache::wr32(uint32_t addr) { access(addr, 4, access_type_t::write); }

void cache::show_stats() {
    std::cout << cache_name << " (" << sets*ways*CACHE_LINE_SIZE << " B): "
              << CACHE_STATS(stats) << std::endl;

    // find n as a largest number of digits - for alignment in stdout
    uint32_t n = 0;
    for (uint32_t set = 0; set < sets; set++) {
        for (uint32_t way = 0; way < ways; way++) {
            uint32_t cnt = cache_entries[set][way].access_cnt;
            uint32_t s = 0;
            while (cnt) {
                cnt /= 10;
                s++;
            }
            if (s > n) n = s;
        }
    }

    for (uint32_t set = 0; set < sets; set++) {
        std::cout << "  s" << set << ": ";
        for (uint32_t way = 0; way < ways; way++) {
            std::cout << " w" << way << " [" << std::setw(n)
                      << cache_entries[set][way].access_cnt << "] ";
        }
        std::cout << std::endl;
    }

    if (roi.start == 0 && roi.end == 0) return;
    std::cout << "  ROI: "
              << "(0x" << std::hex << roi.start
              << " - 0x" << roi.end << "): " << std::dec
              << CACHE_STATS(roi.stats) << std::endl;
}

void cache::log_stats(std::ofstream& log_file) {
    log_file << CACHE_JSON_ENTRY(cache_name, stats) << std::endl;
}

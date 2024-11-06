#include "cache.h"

#define CACHE_LINE_SIZE 64 // bytes
#define CACHE_BYTE_ADDR_BITS (__builtin_ctz(CACHE_LINE_SIZE)) // 6
#define CACHE_BYTE_ADDR_MASK (CACHE_LINE_SIZE - 1) // 0x3F, bottom 6 bits

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

void cache::access(uint32_t addr, access_type_t atype) {
    stats.accesses++;
    //uint32_t byte_addr = address & CACHE_BYTE_ADDR_MASK;
    uint32_t index = (addr >> CACHE_BYTE_ADDR_BITS) & index_mask;
    uint32_t tag = addr >> (CACHE_BYTE_ADDR_BITS + index_bits_num);
    evict_t evict;

    for (uint32_t way = 0; way < ways; way++) {
        auto& line = cache_entries[index][way];
        if (line.metadata.valid && line.tag == tag) {
            line.access_cnt++;
            stats.hits++;
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

    stats.misses++;
    auto& active_line = cache_entries[index][evict.way];
    active_line.access_cnt++;
    // evict the line
    if (active_line.metadata.valid) {
        if (active_line.metadata.dirty) {
            //mem->wr_line(address, line.data);
            active_line.metadata.dirty = false;
            stats.writebacks++;
        }
    }

    // bring in the new line
    //line.data = mem->rd_line(address);
    active_line.tag = tag;
    active_line.metadata.valid = true;
    update_lru(index, evict.way);

    // TODO: prefetcher

    return;
}

// TODO: reads should return in 'func' mode
uint8_t cache::rd8(uint32_t address) {
    access(address, access_type_t::read);
    return 0;
}

uint16_t cache::rd16(uint32_t address) {
    access(address, access_type_t::read);
    return 0;
}

uint32_t cache::rd32(uint32_t address) {
    access(address, access_type_t::read);
    return 0;
}

// TODO: writes should take in data in 'func' mode and write to cache
void cache::wr8(uint32_t address) {
    access(address, access_type_t::write);
}

void cache::wr16(uint32_t address) {
    access(address, access_type_t::write);
}

void cache::wr32(uint32_t address) {
    access(address, access_type_t::write);
}

void cache::show_stats() {
    std::cout << cache_name << " stats: ";
    std::cout << "accesses: " << stats.accesses;
    std::cout << ", hits: " << stats.hits;
    std::cout << ", misses: " << stats.misses;
    std::cout << ", writebacks: " << stats.writebacks;
    // hit rate, rounded to 2 decimal places
    std::cout << ", hit rate: " << std::fixed << std::setprecision(2)
              << (float)stats.hits / stats.accesses * 100 << "%" << std::endl;

    // find n as a largest number of digits before printing
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
}

void cache::log_stats(std::ofstream& log_file) {
    log_file << CACHE_JSON_ENTRY(cache_name, stats.accesses, stats.hits,
                                 stats.misses, stats.writebacks);

    log_file << std::endl;
}

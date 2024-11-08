#pragma once

#include "defines.h"

#define CACHE_LINE_SIZE 64 // bytes
#define CACHE_BYTE_ADDR_BITS (__builtin_ctz(CACHE_LINE_SIZE)) // 6
#define CACHE_BYTE_ADDR_MASK (CACHE_LINE_SIZE - 1) // 0x3F, bottom 6 bits

#define CACHE_JSON_ENTRY(name, stat_struct) \
    "\"" << name << "\"" << ": {" \
    << "\"accesses\": " << stat_struct.accesses \
    << ", \"hits\": " << stat_struct.hits \
    << ", \"misses\": " << stat_struct.misses \
    << ", \"evicts\": " << stat_struct.evicts \
    << ", \"writebacks\": " << stat_struct.writebacks \
    << ", \"bw_core\": {\"reads\": " << stat_struct.bw_core.reads \
    << ", \"writes\": " << stat_struct.bw_core.writes << "}" \
    << ", \"bw_mem\": {\"reads\": " << stat_struct.bw_mem.reads \
    << ", \"writes\": " << stat_struct.bw_mem.writes << "}" \
    << "},"

#define CACHE_STATS(stat_struct) \
    "A: " << stat_struct.accesses \
    << ", H: " << stat_struct.hits \
    << ", M: " << stat_struct.misses \
    << ", E: " << stat_struct.evicts \
    << ", WB: " << stat_struct.writebacks \
    << ", HR: " << std::fixed << std::setprecision(2) \
    << (float)stat_struct.hits/stat_struct.accesses * 100 << "%" \
    << "; BW (R/W): core " \
    << stat_struct.bw_core.reads << "/" << stat_struct.bw_core.writes << " B" \
    << ", mem " \
    << stat_struct.bw_mem.reads << "/" << stat_struct.bw_mem.writes << " B"

enum class access_type_t { read, write };

struct metadata_t {
    bool valid;
    bool dirty;
    // bool scp;
    uint32_t lru_cnt;
    metadata_t() : valid(false), dirty(false), lru_cnt(0) {}
};

struct cache_line_t {
    metadata_t metadata;
    uint32_t tag;
    //#if CACHE_MODE == CACHE_MODE_FUNC
    //std::array<uint8_t, CACHE_LINE_SIZE> data;
    //#endif
    uint32_t access_cnt; // as a utilization of the cache line
    cache_line_t() : metadata(), tag(0), access_cnt(0) {}
};

struct data_bandwidth_t {
    // TODO: this needs to be expanded when used with DPI (i.e. timing sim)
    // and should be able to show memory pressure as time series
    // for now, just keep track of the total data transferred
    uint32_t reads; // bytes
    uint32_t writes;
    data_bandwidth_t() : reads(0), writes(0) {}
};

struct cache_stats_t {
    uint32_t accesses;
    uint32_t hits;
    uint32_t misses;
    uint32_t evicts;
    uint32_t writebacks;
    data_bandwidth_t bw_core;
    data_bandwidth_t bw_mem;
    cache_stats_t() :
        accesses(0), hits(0), misses(0), evicts(0), writebacks(0),
        bw_core(), bw_mem() {}

    void hit(access_type_t atype, uint32_t size) {
        hits++;
        if (atype == access_type_t::read) bw_core.reads += size;
        else bw_core.writes += size;
    }

    void miss() {
        misses++;
        bw_mem.reads += CACHE_LINE_SIZE;
    }

    void writeback() {
        writebacks++;
        bw_mem.writes += CACHE_LINE_SIZE;
    }
};

struct region_of_interest_t {
    uint32_t start;
    uint32_t end;
    cache_stats_t stats;
    region_of_interest_t() : start(0), end(0), stats() {}
    void set(uint32_t start, uint32_t end) {
        this->start = start;
        this->end = end;
    }
    bool has(uint32_t addr) const { return start <= addr && addr <= end; }
};

struct evict_t {
    uint32_t way;
    uint32_t lru_cnt;
    evict_t() : way(0), lru_cnt(0) {}
};

class cache {
    private:
        //memory* mem;
        uint32_t sets;
        uint32_t ways;
        uint32_t index_bits_num;
        uint32_t index_mask;
        uint32_t tag_bits_num;
        std::vector<std::vector<cache_line_t>> cache_entries;
        cache_stats_t stats;
        std::string cache_name;
        region_of_interest_t roi;

    public:
        cache() = delete;
        //cache(uint32_t sets, uint32_t ways, std::string cache_name, memory* mem);
        cache(uint32_t sets, uint32_t ways, std::string cache_name);
        uint8_t rd8(uint32_t addr);
        uint16_t rd16(uint32_t addr);
        uint32_t rd32(uint32_t addr);
        void wr8(uint32_t addr);
        void wr16(uint32_t addr);
        void wr32(uint32_t addr);
        void set_roi(uint32_t start, uint32_t end);
        void show_stats();
        void log_stats(std::ofstream& log_file);

private:
        void access(uint32_t addr, uint32_t size, access_type_t atype);
        void update_lru(uint32_t index, uint32_t way);
        bool is_pow_2(int n) const { return n > 0 && !(n & (n - 1)); }
};

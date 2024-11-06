#pragma once

#include "defines.h"

#define CACHE_JSON_ENTRY(name, accesses, hits, misses, writebacks) \
    "\"" << name << "\"" << ": {" \
    << "\"accesses\": " << accesses \
    << ", \"hits\": " << hits \
    << ", \"misses\": " << misses \
    << ", \"writebacks\": " << writebacks \
    << "},"

struct metadata_t {
    bool valid;
    bool dirty;
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

struct cache_stats_t {
    uint32_t accesses;
    uint32_t hits;
    uint32_t misses;
    uint32_t writebacks;
    cache_stats_t() : accesses(0), hits(0), misses(0), writebacks(0) {}
};

enum class access_type_t { read, write };

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
        void show_stats();
        void log_stats(std::ofstream& log_file);

private:
        void access(uint32_t addr, access_type_t atype);
        void update_lru(uint32_t index, uint32_t way);
        bool is_pow_2(int n) const { return n > 0 && !(n & (n - 1)); }
};

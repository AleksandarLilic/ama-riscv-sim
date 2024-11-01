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
    metadata_t() : valid(false), dirty(false) {}
};

struct cache_block_t {
    metadata_t metadata;
    uint32_t tag;
    //#if CACHE_MODE == CACHE_MODE_FUNC
    //std::array<uint8_t, CACHE_BLOCK_SIZE> data;
    //#endif
    cache_block_t() : metadata(), tag(0) {}
};

struct cache_stats_t {
    uint32_t accesses;
    uint32_t hits;
    uint32_t misses;
    uint32_t writebacks;
    cache_stats_t() : accesses(0), hits(0), misses(0), writebacks(0) {}
};

enum class access_type_t { read, write };

class cache {
    private:
        //memory* mem;
        size_t block_num;
        size_t index_bits;
        size_t index_mask;
        std::vector<cache_block_t> cache_data;
        cache_stats_t stats;
        std::string cache_name;

    public:
        cache() = delete;
        //cache(memory* mem, size_t block_num);
        cache(size_t block_num, std::string cache_name);
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
        bool is_pow_2(int n) const { return n > 0 && !(n & (n - 1)); }
};

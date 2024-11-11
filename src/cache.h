#pragma once

#include "defines.h"

#define CACHE_LINE_SIZE 64 // bytes
#define CACHE_BYTE_ADDR_BITS (__builtin_ctz(CACHE_LINE_SIZE)) // 6
#define CACHE_BYTE_ADDR_MASK (CACHE_LINE_SIZE - 1) // 0x3F, bottom 6 bits
//#define CACHE_ADDR_MASK 1FFFF // 17 bits

#define CACHE_JSON_ENTRY(name, stat_struct) \
    "\"" << name << "\"" << ": {" \
    << "\"accesses\": " << stat_struct->accesses \
    << ", \"hits\": " << stat_struct->hits \
    << ", \"misses\": " << stat_struct->misses \
    << ", \"evicts\": " << stat_struct->evicts \
    << ", \"writebacks\": " << stat_struct->writebacks \
    << ", \"bw_core\": {\"reads\": " << stat_struct->bw_core.reads \
    << ", \"writes\": " << stat_struct->bw_core.writes << "}" \
    << ", \"bw_mem\": {\"reads\": " << stat_struct->bw_mem.reads \
    << ", \"writes\": " << stat_struct->bw_mem.writes << "}" \
    << "},"

struct metadata_t {
    bool valid;
    bool dirty;
    bool scp;
    //bool speculative; // speculative load, evict on mispred (ignore lru)
    uint32_t lru_cnt;
    metadata_t() : valid(false), dirty(false), scp(false), lru_cnt(0) {}
    static uint32_t get_bits_num() { return 3; } // update if more flags added
};

struct cache_line_t {
    public:
    metadata_t metadata;
    uint32_t tag;
    #if CACHE_MODE == CACHE_MODE_FUNC
    std::array<uint8_t, CACHE_LINE_SIZE> data;
    #endif
    //private:
    uint32_t access_cnt;
    //std::array<uint32_t, CACHE_LINE_SIZE> byte_access_cnt;
    public:
    cache_line_t() : metadata(), tag(0), access_cnt(0) {}
    void access() {
        access_cnt++;
        //for (uint32_t i = 0; i < size; i++) {
        //    byte_access_cnt[(addr + i) & CACHE_BYTE_ADDR_MASK]++;
        //}
    }
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
    private:
    uint32_t hits;
    uint32_t misses;
    uint32_t evicts;
    uint32_t writebacks;
    data_bandwidth_t bw_core;
    data_bandwidth_t bw_mem;
    public:
    cache_stats_t() :
        accesses(0), hits(0), misses(0), evicts(0), writebacks(0),
        bw_core(), bw_mem() {}

    void access(access_t atype, uint32_t size) {
        accesses++;
        if (atype == access_t::read) bw_core.reads += size;
        else bw_core.writes += size;
    }
    void hit() { hits++; }
    void miss() {
        misses++;
        bw_mem.reads += CACHE_LINE_SIZE;
    }
    void evict(bool dirty) {
        evicts++;
        if (dirty) writeback();
    }
    private:
    void writeback() {
        writebacks++;
        bw_mem.writes += CACHE_LINE_SIZE;
    }
    public:
    void show() const {
        std::cout << "A: " << accesses
                  << ", H: " << hits
                  << ", M: " << misses
                  << ", E: " << evicts
                  << ", WB: " << writebacks
                  << ", HR: " << std::fixed << std::setprecision(2)
                  << TO_F32(hits)/TO_F32(accesses) * 100 << "%"
                  << "; BW (R/W): "
                  << "core " << bw_core.reads << "/"<< bw_core.writes << " B"
                  << ", mem " << bw_mem.reads << "/" << bw_mem.writes << " B";
    }
    void log(std::string name, std::ofstream& log_file) const {
        log_file << CACHE_JSON_ENTRY(name, this) << std::endl;
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

struct target_t {
    uint32_t way_idx;
    uint32_t lru_cnt;
    target_t() : way_idx(0), lru_cnt(0) {}
    target_t(uint32_t way, uint32_t lru_cnt) : way_idx(way), lru_cnt(lru_cnt) {}
};

class main_memory; // forward declaration

/*
Cache parameters (P: parametrized, D: derived, S: single option):
- line size (P): number of bytes in a cache line
- sets (P): number of sets in the cache
- ways (P): associativity - number of ways in each set
- size (D): sets * ways * CACHE_LINE_SIZE
- replacement policy (S): LRU
- write policy (S): write-back
- write-allocate (S): yes
- prefetching (S): no
- inclusive/exclusive (S): not applicable, no L2 cache
- cache coherence (S): not applicable, single core
*/
class cache {
    private:
        uint32_t sets;
        uint32_t ways;
        uint32_t index_bits_num;
        uint32_t index_mask;
        uint32_t tag_bits_num;
        uint32_t tag_off;
        uint32_t metadata_bits_num;
        std::vector<std::vector<cache_line_t>> cache_entries;
        cache_stats_t stats;
        std::string cache_name;
        main_memory* mem;
        region_of_interest_t roi;
        uint32_t rd_buf;
        uint32_t wr_buf;
        uint32_t max_scp;
        scp_status_t scp_status;

    public:
        cache() = delete;
        cache(uint32_t sets, uint32_t ways,
              std::string cache_name, main_memory* mem);
        uint32_t rd(uint32_t addr, uint32_t size);
        void wr(uint32_t addr, uint32_t data, uint32_t size);
        scp_status_t scp_ld(uint32_t addr);
        scp_status_t scp_rel(uint32_t addr);
        // stats
        void set_roi(uint32_t start, uint32_t end);
        void show_stats();
        void log_stats(std::ofstream& log_file);
        void dump() const;

private:
        void access(uint32_t addr, uint32_t size,
                    access_t atype, scp_mode_t scp);
        void update_lru(uint32_t index, uint32_t way);
        scp_status_t update_scp(scp_mode_t mode, cache_line_t& line,
                                uint32_t index);
        scp_status_t convert_to_scp(cache_line_t& line, uint32_t index);
        scp_status_t release_scp(cache_line_t& line);
        bool is_pow_2(uint32_t n) const { return n > 0 && !(n & (n - 1)); }
        #if CACHE_MODE == CACHE_MODE_FUNC
        void read_from_cache(uint32_t byte_addr, uint32_t size,
                             cache_line_t& act_line);
        void write_to_cache(uint32_t byte_addr, uint32_t size,
                            cache_line_t& act_line);
        #endif
};

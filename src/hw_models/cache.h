#pragma once

#include "defines.h"
#include "cache_stats.h"
#include "profiler_perf.h"

//#ifdef DPI
//#define MEM_TYPE memory_dpi
//#else
#define MEM_TYPE main_memory
//#endif

#define MAX_CACHE_SETS 1024
#define MAX_CACHE_WAYS 128

struct metadata_t {
    bool valid;
    bool dirty;
    bool scp;
    //bool speculative; // line brought in during speculative execution
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
        uint32_t reference_cnt;
        //std::array<uint32_t, CACHE_LINE_SIZE> byte_reference_cnt;
    private:
        bool prof_active = false;
    public:
        cache_line_t() : metadata(), tag(0), reference_cnt(0) {}
        void profiling(bool enable) { prof_active = enable; }
        void referenced() {
            if (!prof_active) return;
            reference_cnt++;
            //for (uint32_t i = 0; i < size; i++) {
            //    byte_reference_cnt[(addr + i) & CACHE_BYTE_ADDR_MASK]++;
            //}
        }
};

struct victim_t {
    uint32_t way_idx;
    uint32_t lru_cnt;
    victim_t() : way_idx(0), lru_cnt(0) {}
    victim_t(uint32_t way, uint32_t lru_cnt) : way_idx(way), lru_cnt(lru_cnt) {}
};

struct current_cache_line {
    uint32_t index;
    uint32_t tag;
    victim_t victim;
    uint32_t byte_addr;
    public:
        void init(uint32_t index, uint32_t tag, victim_t victim) {
            this->index = index;
            this->tag = tag;
            this->victim = victim;
        }
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
        cache_policy_t policy;
        uint32_t index_bits_num;
        uint32_t index_mask;
        uint32_t tag_bits_num;
        uint32_t tag_off;
        current_cache_line ccl;
        uint32_t metadata_bits_num;
        std::vector<std::vector<cache_line_t>> cache_entries;
        std::string cache_name;
        MEM_TYPE* mem;
        cache_stats_t stats;
        cache_size_t size;
        region_of_interest_t roi;
        uint32_t rd_buf;
        uint32_t wr_buf;
        uint32_t max_scp_ways;
        scp_status_t scp_status;
        speculative_t smode;
        bool speculative_exec_active; // not used atm
        hw_status_t* hws;
        #ifdef PROFILERS_EN
        profiler_perf* prof_perf;
        perf_event_t ref_event;
        perf_event_t miss_event;
        #endif

    public:
        cache() = delete;
        cache(
            uint32_t sets, uint32_t ways, cache_policy_t policy,
            std::string cache_name);
        void set_mem(MEM_TYPE* mem) { this->mem = mem; }
        uint32_t rd(uint32_t addr, uint32_t size);
        void wr(uint32_t addr, uint32_t data, uint32_t size);
        scp_status_t scp_lcl(uint32_t addr);
        scp_status_t scp_rel(uint32_t addr);
        void speculative_exec(speculative_t smode);
        void set_hws(hw_status_t* hws) { this->hws = hws; };

        // prof
        void profiling(bool enable) {
            stats.profiling(enable);
            roi.stats.profiling(enable);
            for (auto& set : cache_entries) {
                for (auto& line : set) line.profiling(enable);
            }
        }
        #ifdef PROFILERS_EN
        void set_perf_profiler(
            profiler_perf* prof_perf,
            perf_event_t ref_event,
            perf_event_t miss_event)
        {
            this->prof_perf = prof_perf;
            this->ref_event = ref_event;
            this->miss_event = miss_event;
        }
        #endif

        // stats
        void set_roi(uint32_t start, uint32_t size);
        void show_stats();
        void log_stats(std::ofstream& log_file);
        float_t get_hr() { return stats.get_hr(); }
        void dump() const;

    private:
        cache_ref_t reference(
            uint32_t addr, uint32_t size, mem_op_t atype, scp_mode_t scp);
        void miss(
            uint32_t addr, uint32_t size, mem_op_t atype, scp_mode_t scp);
        void update_lru(uint32_t index, uint32_t way);
        scp_status_t update_scp(
            scp_mode_t mode, cache_line_t& line,uint32_t index);
        scp_status_t convert_to_scp(cache_line_t& line, uint32_t index);
        scp_status_t release_scp(cache_line_t& line);
        bool is_pow_2(uint32_t n) const { return n > 0 && !(n & (n - 1)); }
        void validate_inputs(
            uint32_t sets, uint32_t ways, cache_policy_t policy);
        #if CACHE_MODE == CACHE_MODE_FUNC
        void read_from_cache(
            uint32_t byte_addr, uint32_t size, cache_line_t& act_line);
        void write_to_cache(
            uint32_t byte_addr, uint32_t size, cache_line_t& act_line);
        #endif
};

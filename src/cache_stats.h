#pragma once

#include "defines.h"

#define CACHE_STATS_JSON_ENTRY(stat_struct) \
    "\"accesses\": " << stat_struct->accesses \
    << ", \"hits\": " << stat_struct->hits \
    << ", \"misses\": " << stat_struct->misses \
    << ", \"evicts\": " << stat_struct->evicts \
    << ", \"writebacks\": " << stat_struct->writebacks \
    << ", \"bw_core\": {\"reads\": " << stat_struct->bw_core.reads \
    << ", \"writes\": " << stat_struct->bw_core.writes << "}" \
    << ", \"bw_mem\": {\"reads\": " << stat_struct->bw_mem.reads \
    << ", \"writes\": " << stat_struct->bw_mem.writes << "}" \

#define CACHE_SIZE_JSON_ENTRY(size_struct) \
    "\"size\"" << ": {" \
    << "\"data\": " << size_struct->data \
    << ", \"tags\": " << size_struct->tags \
    << ", \"metadata\": " << size_struct->metadata \
    << ", \"sets\": " << size_struct->sets \
    << ", \"ways\": " << size_struct->ways \
    << ", \"line_size\": " << size_struct->line_size \
    << "}"

struct cache_size_t {
    private:
        uint32_t data;
        uint32_t tags;
        uint32_t metadata;
        uint32_t sets;
        uint32_t ways;
        uint32_t line_size;

    public:
        cache_size_t() : data(0), tags(0), metadata(0),
                         sets(0), ways(0), line_size(0) {}
        cache_size_t(uint32_t sets, uint32_t ways, uint32_t line_size,
                     uint32_t tag_bits_num, uint32_t metadata_bits_num) {
            this->sets = sets;
            this->ways = ways;
            this->line_size = line_size;
            data = sets * ways * line_size;
            tags = ((sets * ways * tag_bits_num) >> 3) + 1;
            metadata = ((sets * ways * metadata_bits_num) >> 3) + 1;
        }
        void show() const {
            std::cout << " (W/S: " << ways << "/" << sets
                      << ", D/T/M: " << data << "/" << tags << "/" << metadata
                      << " B): ";
        }
        void log(std::ofstream& log_file) const {
            log_file << CACHE_SIZE_JSON_ENTRY(this);
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
    private:
        uint32_t accesses;
        uint32_t hits;
        uint32_t misses;
        uint32_t evicts;
        uint32_t writebacks;
        data_bandwidth_t bw_core;
        data_bandwidth_t bw_mem;
        bool prof_active = false;

    public:
        cache_stats_t() :
            accesses(0), hits(0), misses(0), evicts(0), writebacks(0),
            bw_core(), bw_mem() {}

        void profiling(bool enable) { prof_active = enable; }
        void access(access_t atype, uint32_t size) {
            if (!prof_active) return;
            accesses++;
            if (atype == access_t::read) bw_core.reads += size;
            else bw_core.writes += size;
        }
        void hit() {
            if (!prof_active) return;
            hits++;
        }
        void miss() {
            if (!prof_active) return;
            misses++;
            bw_mem.reads += CACHE_LINE_SIZE;
        }
        void evict(bool dirty) {
            if (!prof_active) return;
            evicts++;
            if (dirty) writeback();
        }

    private:
        void writeback() {
            if (!prof_active) return;
            writebacks++;
            bw_mem.writes += CACHE_LINE_SIZE;
        }

    public:
        void show() const {
            float_t hit_rate = 0.0;
            if (accesses > 0) hit_rate = TO_F32(hits) / TO_F32(accesses) * 100;
            std::cout << "A: " << accesses
                    << ", H: " << hits
                    << ", M: " << misses
                    << ", E: " << evicts
                    << ", WB: " << writebacks
                    << ", HR: " << std::fixed << std::setprecision(2)
                    << hit_rate << "%"
                    << "; BW (R/W): "
                    << "core " << bw_core.reads << "/"<< bw_core.writes << " B"
                    << ", mem " << bw_mem.reads << "/" << bw_mem.writes << " B";
        }
        void log(std::ofstream& log_file) const {
            log_file << CACHE_STATS_JSON_ENTRY(this);
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

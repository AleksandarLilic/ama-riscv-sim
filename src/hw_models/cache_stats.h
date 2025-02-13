#pragma once

#include "defines.h"

#define CACHE_STATS_JSON_ENTRY(stat_struct) \
    "\"references\": " << stat_struct->references \
    << ", \"hits\": " << stat_struct->hits \
    << ", \"misses\": " << stat_struct->misses \
    << ", \"evicts\": " << stat_struct->evicts \
    << ", \"writebacks\": " << stat_struct->writebacks \
    << ", \"ct_core\": {\"reads\": " << stat_struct->ct_core.reads \
    << ", \"writes\": " << stat_struct->ct_core.writes << "}" \
    << ", \"ct_mem\": {\"reads\": " << stat_struct->ct_mem.reads \
    << ", \"writes\": " << stat_struct->ct_mem.writes << "}" \

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

#define PREC_THR 10
struct cache_traffic_t {
    // TODO: this needs to be expanded when used with DPI (i.e. timing sim)
    // and should be able to show memory pressure as time series
    // for now, just keep track of the total data transferred
    uint32_t reads; // bytes
    uint32_t writes;
    cache_traffic_t() : reads(0), writes(0) {}
    std::string to_string() const {
        float rd = TO_F32(reads);
        float wr = TO_F32(writes);
        std::string suffixes[] = {"B", "KB", "MB", "GB", "TB"};
        size_t idx = 0;
        while (rd >= 1024 || wr >= 1024) {
            rd /= 1024;
            wr /= 1024;
            idx++;
        }
        size_t prec = ((rd < PREC_THR) || (wr < PREC_THR)) ? 1 : 0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(prec)
            << rd << "/" << wr << " " << suffixes[idx];
        return oss.str();
    }
};

struct cache_stats_t {
    private:
        uint32_t references;
        uint32_t hits;
        uint32_t misses;
        uint32_t evicts;
        uint32_t writebacks;
        cache_traffic_t ct_core;
        cache_traffic_t ct_mem;
        bool prof_active = false;

    public:
        cache_stats_t() :
            references(0), hits(0), misses(0), evicts(0), writebacks(0),
            ct_core(), ct_mem() {}

        void profiling(bool enable) { prof_active = enable; }
        void reference(mem_op_t atype, uint32_t size) {
            if (!prof_active) return;
            references++;
            if (atype == mem_op_t::read) ct_core.reads += size;
            else ct_core.writes += size;
        }
        void hit() {
            if (!prof_active) return;
            hits++;
        }
        void miss() {
            if (!prof_active) return;
            misses++;
            ct_mem.reads += CACHE_LINE_SIZE;
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
            ct_mem.writes += CACHE_LINE_SIZE;
        }

    public:
        void show() const {
            float_t hr = 0.0; // hit rate
            if (references > 0) hr = TO_F32(hits) / TO_F32(references) * 100;
            std::cout << "R: " << references
                      << ", H: " << hits
                      << ", M: " << misses
                      << ", E: " << evicts
                      << ", WB: " << writebacks
                      << ", HR: " << std::fixed << std::setprecision(2)
                      << hr << "%"
                      << "; CT (R/W): "
                      << "core " << ct_core.to_string()
                      << ", mem " << ct_mem.to_string();
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
    void set(uint32_t start, uint32_t size) {
        this->start = start;
        this->end = start + size;
    }
    bool has(uint32_t addr) const { return start <= addr && addr <= end; }
};

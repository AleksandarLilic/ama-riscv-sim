#pragma once

#include "defines.h"

// caches
enum class hw_status_t { miss, hit, none };
enum class cache_policy_t { lru, _count };
enum class cache_ref_t { hit, miss, ignore, _count };
enum class scp_mode_t { m_none, m_lcl, m_rel };
// success always 0, fail 1 for now, use values >0 for error codes if needed
enum class scp_status_t { success, fail };
enum class speculative_t { enter, exit_commit, exit_flush };
enum class cache_access_reason_t {
    data_read, inst_read, speculative, resolved, _count};

struct cache_access_stat {
    std::string name;
    uint32_t addr;
    uint32_t tag;
    uint32_t index;
    uint32_t way;
    uint32_t byte_addr;
    mem_op_t atype;
    //cache_access_reason_t reason;
    bool is_scp;
    bool is_hit;
};

struct bp_access_stat {
    std::string name;
    b_dir_t dir;
    bool hit;
    bool taken;
};

// branches and BP
enum class bp_t {sttc, bimodal, local, global, gselect, gshare,
                 ideal, none, combined, _count };
enum class bp_sttc_t { at, ant, btfn, _count };
enum class bp_bits_t { pc, cnt, hist, gr, _count };
enum class bp_pc_folds_t { none, all, _count };

struct bp_cfg_t {
    public:
        const uint8_t pc_bits;
        const uint8_t cnt_bits;
        const uint8_t hist_bits;
        const uint8_t gr_bits;
        const bp_pc_folds_t fold_pc;
        //const std::string type_name;
        const char* type_name;
};

// common
struct hw_running_stats_t {
    hw_status_t ic_hm;
    hw_status_t dc_hm;
    hw_status_t bp_hm;
    void rst() {
        ic_hm = hw_status_t::none;
        dc_hm = hw_status_t::none;
        bp_hm = hw_status_t::none;
    }
};

struct hwmi_str { // hardware model info
    private:
        std::ostringstream stat_ss;
        std::string out_str;
        //std::array<std::string, TO_U32(cache_access_reason_t::_count)>
        //cache_access_reason = {
        //    "data read", "inst read", "speculative", "resolved"
        //};
    public:
        void log_cache(cache_access_stat cas) {
            stat_ss << "HW - " << cas.name
                    << " (" << (cas.is_scp ? "S" : "C" )
                    << ") : " << (cas.is_hit ? "HIT " : "MISS")
                    << " (" << ((cas.atype == mem_op_t::write) ? "W" : "R" )
                    << ") [A:0x" << MEM_ADDR_FORMAT(cas.addr)
                    << ", W:" << cas.way
                    << ", T:" << FHEXZ(cas.tag, 4)
                    << ", I:" << cas.index
                    << ", B:" << std::setw(2) << std::setfill('0')
                    << cas.byte_addr << "]; ";
        }
        void log_bp(bp_access_stat bpas) {
            stat_ss << "HW - " << bpas.name << ": "
                    << (bpas.hit ? "HIT" : "MISS") << " ["
                    << (bpas.taken ? "T" : "N") << ", "
                    << (bpas.dir == b_dir_t::backward ? "BWD" : "FWD")
                    << "]; ";
        }
        //void log_bp(bp_access_stat bal) { }
        void clear_str() { stat_ss.str(""); }
        std::string get_str() {
            out_str = stat_ss.str();
            return out_str;
        }
};

struct hw_cfg_t {
    // caches
    uint32_t icache_sets;
    uint32_t icache_ways;
    cache_policy_t icache_policy;
    uint32_t dcache_sets;
    uint32_t dcache_ways;
    cache_policy_t dcache_policy;
    uint32_t roi_start;
    uint32_t roi_size;
    bool show_cache_state;
    // branch predictors
    bp_t bp;
    bp_t bp2;
    bp_t bp_active;
    std::string bp_active_name;
    // supported predictors configurations
    bp_sttc_t bp_static_method;
    uint8_t bp_pc_bits;
    uint8_t bp_cnt_bits;
    uint8_t bp_lhist_bits;
    uint8_t bp_gr_bits;
    bp_pc_folds_t bp_fold_pc;
    bp_sttc_t bp2_static_method;
    uint8_t bp2_pc_bits;
    uint8_t bp2_cnt_bits;
    uint8_t bp2_lhist_bits;
    uint8_t bp2_gr_bits;
    bp_pc_folds_t bp2_fold_pc;
    uint8_t bp_combined_pc_bits;
    uint8_t bp_combined_cnt_bits;
    bp_pc_folds_t bp_combined_fold_pc;
    // bp other configs
    bool bp_run_all; // optionally, run all predefined predictors
    bool bp_dump_csv;
};

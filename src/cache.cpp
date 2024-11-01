#include "cache.h"

#define CACHE_BLOCK_SIZE 64 // bytes
#define CACHE_BYTE_ADDR_BITS (__builtin_ctz(CACHE_BLOCK_SIZE)) // 6
#define CACHE_BYTE_ADDR_MASK (CACHE_BLOCK_SIZE - 1) // 0x3F, bottom 6 bits


cache::cache(size_t block_num, std::string cache_name)
    : block_num(block_num), cache_name(cache_name) {
    if (!is_pow_2(block_num)) {
        std::string err_msg =
            "Number of blocks must be a power of 2. Specified: " +
            std::to_string(block_num);
        throw std::runtime_error(err_msg);
    }

    cache_data.resize(block_num);
    index_bits = 0;
    index_mask = 0;
    for (size_t i = block_num; i > 1; i >>= 1) {
        index_bits++;
        index_mask = (index_mask << 1) | 1;
    }
}

void cache::access(uint32_t addr, access_type_t atype) {
    stats.accesses++;
    //uint32_t byte_addr = address & CACHE_BYTE_ADDR_MASK;
    uint32_t index = (addr >> CACHE_BYTE_ADDR_BITS) & index_mask;
    uint32_t tag = addr >> (CACHE_BYTE_ADDR_BITS + index_bits);

    // TODO: current implementation is direct-mapped
    // other policies have to implemented, and called based on cli args

    cache_block_t& block = cache_data[index]; // TODO: can have multiple ways
    if (block.metadata.valid && block.tag == tag) {
        stats.hits++;
        if (atype == access_type_t::write) {
            // TODO: needs write policy (write-through, write-back)
            block.metadata.dirty = true;
        }
        return;
    }

    stats.misses++;
    if (block.metadata.valid) {
        // TODO: eviction policy
        if (block.metadata.dirty) {
            //mem->wr_block(address, block.data);
            block.metadata.dirty = false;
            stats.writebacks++;
        }
    }

    //block.data = mem->rd_block(address);
    block.tag = tag;
    block.metadata.valid = true;

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
}

void cache::log_stats(std::ofstream& log_file) {
    log_file << CACHE_JSON_ENTRY(cache_name, stats.accesses, stats.hits,
                                 stats.misses, stats.writebacks);

    log_file << std::endl;
}

#include "main_memory.h"

main_memory::main_memory(
    size_t size,
    std::string test_bin,
    [[maybe_unused]] hw_cfg_t hw_cfg) :
        dev(size)
        #ifdef ENABLE_HW_PROF
        ,
        icache(hw_cfg.icache_sets, hw_cfg.icache_ways, "icache", this),
        dcache(hw_cfg.dcache_sets, hw_cfg.dcache_ways, "dcache", this)
        #endif
{
    burn(test_bin);
    #ifdef ENABLE_HW_PROF
    dcache.set_roi(hw_cfg.roi_start, hw_cfg.roi_size);
    #endif
}

void main_memory::burn(std::string test_bin) {
    std::ifstream bin_file(test_bin, std::ios::binary | std::ios::ate);
    if (!bin_file.is_open()) {
        std::cerr << "ERROR: Failed to open binary file: " << test_bin
                  << std::endl;
        throw std::runtime_error("BIN failed to open.");
    }

    size_t file_size = bin_file.tellg();
    if (file_size > MEM_SIZE) {
        std::cerr << "ERROR: File size is greater than memory size."
                  << " Binary size: " << file_size << "B"
                  << " Memory size: " << MEM_SIZE << "B"
                  << " Binary not loaded" << std::endl;
        throw std::runtime_error("File size is greater than memory size.");
    }

    bin_file.seekg(0, std::ios::beg);
    bin_file.read(reinterpret_cast<char*>(mem.data()), file_size);
    bin_file.close();
}

uint32_t main_memory::rd_inst(uint32_t addr) {
    uint32_t inst = dev::rd(addr, 4);
    #ifdef ENABLE_HW_PROF
    #if CACHE_MODE == CACHE_MODE_FUNC and defined(CACHE_VERIFY)
    uint32_t inst_ic = icache.rd(BASE_ADDR + addr, 4);
    if (inst_ic != inst) {
        std::cerr << "ERROR: Instruction cache and memory mismatch."
                  << " Address: 0x" << std::hex << addr
                  << " icache: 0x" << inst_ic
                  << " Memory: 0x" << inst
                  << std::endl;
        icache.dump();
        throw std::runtime_error("Instruction cache and memory mismatch.");
    }
    #else
    icache.rd(BASE_ADDR + addr, 4);
    #endif
    #endif
    return inst;
}

#ifdef ENABLE_HW_PROF
scp_status_t main_memory::scp(uint32_t addr, scp_mode_t scp_mode) {
    addr += BASE_ADDR;
    if (scp_mode == scp_mode_t::m_lcl) return dcache.scp_lcl(addr);
    else if (scp_mode == scp_mode_t::m_rel) return dcache.scp_rel(addr);
    else throw std::runtime_error("ERROR: Invalid cache hint mode");
}
#endif

uint32_t main_memory::rd(uint32_t addr, uint32_t size) {
    uint32_t data = dev::rd(addr, size);
    #ifdef ENABLE_HW_PROF
    #if CACHE_MODE == CACHE_MODE_FUNC and defined(CACHE_VERIFY)
    uint32_t data_dc = dcache.rd(BASE_ADDR + addr, size);
    if (data_dc != data) {
        std::cerr << "ERROR: Data cache and memory mismatch."
                  << " Address: 0x" << std::hex << addr
                  << " dcache: 0x" << data_dc
                  << " Memory: 0x" << data
                  << std::endl;
        dcache.dump();
        throw std::runtime_error("Data cache and memory mismatch.");
    }
    #else
    dcache.rd(BASE_ADDR + addr, size);
    #endif
    #endif
    return data;
}

void main_memory::wr(uint32_t addr, uint32_t data, uint32_t size) {
    #ifdef ENABLE_HW_PROF
    dcache.wr(BASE_ADDR + addr, data, size);
    #endif
    dev::wr(addr, data, size);
}

// TODO: DPI modes should be also added
#if CACHE_MODE == CACHE_MODE_FUNC
std::array<uint8_t, CACHE_LINE_SIZE> main_memory::rd_line(uint32_t addr) {
    std::array<uint8_t, CACHE_LINE_SIZE> data;
    addr = addr & ~CACHE_BYTE_ADDR_MASK; // align to cache line
    for (uint32_t i = 0; i < CACHE_LINE_SIZE; i++) data[i] = dev::rd(addr+i, 1);
    return data;
}
#endif

#if CACHE_MODE == CACHE_MODE_FUNC and defined(CACHE_VERIFY)
void main_memory::wr_line(uint32_t addr,
                          std::array<uint8_t, CACHE_LINE_SIZE> data) {
    addr = addr & ~CACHE_BYTE_ADDR_MASK; // align to cache line
    // don't actually write to memory (the updated data is already there),
    // instead read each byte and compare
    // data in the memory has to be the same as the data in the cache
    for (uint32_t i = 0; i < CACHE_LINE_SIZE; i++) {
        uint32_t mem_data = TO_U32(dev::rd(addr+i, 1));
        uint32_t cache_data = TO_U32(data[i]);
        if (mem_data != cache_data) {
            std::cerr << "ERROR: Data cache and memory mismatch."
                      << " Address: 0x" << std::hex << addr+i
                      << " dcache: 0x" << cache_data
                      << " Memory: 0x" << mem_data
                      << std::endl;
            dcache.dump();
            throw std::runtime_error("Data cache and memory mismatch.");
        }
    }
}
#endif

#include "main_memory.h"

#define ICACHE_CFG \
    hw_cfg.icache_sets, hw_cfg.icache_ways, hw_cfg.icache_policy, "icache"
#define DCACHE_CFG \
    hw_cfg.dcache_sets, hw_cfg.dcache_ways, hw_cfg.dcache_policy, "dcache"

main_memory::main_memory(
    size_t size,
    std::string test_elf,
    [[maybe_unused]] hw_cfg_t hw_cfg) :
        dev(size)
        #ifdef HW_MODELS_EN
        ,
        icache(ICACHE_CFG),
        dcache(DCACHE_CFG)
        #endif
{
    burn_elf(test_elf);
    #ifdef HW_MODELS_EN
    dcache.set_roi(hw_cfg.roi_start, hw_cfg.roi_size);
    #if CACHE_MODE == CACHE_MODE_FUNC
    icache.set_mem(this);
    dcache.set_mem(this);
    #endif
    #endif
}

void main_memory::burn_bin(std::string test_bin) {
    std::ifstream bin_file(test_bin, std::ios::binary | std::ios::ate);
    if (!bin_file.is_open()) {
        std::cerr << "ERROR: Failed to open binary file: " << test_bin
                  << std::endl;
        throw std::runtime_error("Failed to open binary file.");
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

void main_memory::burn_elf(std::string test_elf) {
    ELFIO::elfio reader;
    if (!reader.load(test_elf)) {
        throw std::runtime_error("Failed to load ELF file.");
    }

    // load segment
    ELFIO::segment* load_seg = nullptr;
    for (const auto& seg : reader.segments) {
        if (seg->get_type() == ELFIO::PT_LOAD) {
            load_seg = seg.get();
            break;
        }
    }

    if (load_seg == nullptr) {
        std::cerr << "ERROR: No loadable segment found in ELF file."
                  << " ELF file: " << test_elf
                  << std::endl;
        throw std::runtime_error("No loadable segment found in ELF file.");
    }

    uint64_t addr = load_seg->get_physical_address();
    if (addr != BASE_ADDR) {
        std::cerr << "ERROR: Segment address is not at base address."
                  << " Address: 0x" << std::hex << addr
                  << " Base address: 0x" << BASE_ADDR << std::dec
                  << std::endl;
        throw std::runtime_error(
            "Segment address is not at base address.");
    }

    uint64_t size = load_seg->get_file_size();
    if (size > MEM_SIZE) {
        std::cerr << "ERROR: Segment size is greater than memory size."
                  << " Segment size: 0x" << std::hex << size << " B"
                  << " Memory size: 0x" << std::hex << MEM_SIZE << " B"
                  << std::dec << std::endl;
        throw std::runtime_error(
            "Segment size is greater than memory size.");
    }

    const char* data = load_seg->get_data();
    std::memcpy(mem.data(), data, size);

    #ifdef PROFILERS_EN
    // generate symbol map
    for (size_t i = 0; i < reader.sections.size(); i++) {
        ELFIO::section* sec = reader.sections[i];
        if (sec->get_type() == ELFIO::SHT_SYMTAB) {
            ELFIO::symbol_section_accessor symbols(reader, sec);
            for (size_t j = 0; j < symbols.get_symbols_num(); j++) {
                std::string name;
                ELFIO::Elf64_Addr value = 0;
                ELFIO::Elf_Xword size  = 0;
                unsigned char bind = 0;
                unsigned char type = 0;
                ELFIO::Elf_Half section_index = 0;
                unsigned char other = 0;
                symbols.get_symbol(
                    j, name, value, size, bind, type, section_index, other);

                if (name == "") continue;
                if (value < BASE_ADDR) continue;
                if (name[0] == '$') continue;
                if (section_index == 7) continue;
                // overwrites the symbol if it already exists
                symbol_map[TO_U32(value)] = {0, name};
            }
            break;
        }
    }

    // TODO: consider increasing the limit
    if (symbol_map.size() > 255) {
        std::cerr << "ERROR: Number of symbols is greater than 255."
                  << " Number of symbols: " << symbol_map.size()
                  << std::endl;
        throw std::runtime_error("Number of symbols is greater than 255.");
    }

    // re-index the symbols
    uint8_t idx = 1; // 0th index is reserved
    for (const auto& sym : symbol_map) symbol_map[sym.first].idx = idx++;

    //for (const auto& sym : symbol_map) {
    //    std::cout << TO_U32(sym.second.idx) << " " << sym.second.name
    //              << " 0x" << std::hex << sym.first << std::dec << "\n";
    //}
    #endif
}

uint32_t main_memory::rd_inst(uint32_t addr) {
    uint32_t inst = dev::rd(addr, 4);
    #ifdef HW_MODELS_EN
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

#ifdef HW_MODELS_EN
scp_status_t main_memory::scp(uint32_t addr, scp_mode_t scp_mode) {
    addr += BASE_ADDR;
    if (scp_mode == scp_mode_t::m_lcl) return dcache.scp_lcl(addr);
    else if (scp_mode == scp_mode_t::m_rel) return dcache.scp_rel(addr);
    else throw std::runtime_error("ERROR: Invalid cache hint mode");
}
#endif

uint32_t main_memory::rd(uint32_t addr, uint32_t size) {
    uint32_t data = dev::rd(addr, size);
    #ifdef HW_MODELS_EN
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
    #ifdef HW_MODELS_EN
    dcache.wr(BASE_ADDR + addr, data, size);
    #endif
    dev::wr(addr, data, size);
}

#if CACHE_MODE == CACHE_MODE_FUNC
std::array<uint8_t, CACHE_LINE_SIZE> main_memory::rd_line(uint32_t addr) {
    std::array<uint8_t, CACHE_LINE_SIZE> data;
    addr = addr & ~CACHE_BYTE_ADDR_MASK; // align to cache line
    for (uint32_t i = 0; i < CACHE_LINE_SIZE; i++) data[i] = dev::rd(addr+i, 1);
    return data;
}
#endif

#if CACHE_MODE == CACHE_MODE_FUNC and defined(CACHE_VERIFY)
void main_memory::wr_line(
    uint32_t addr, std::array<uint8_t, CACHE_LINE_SIZE> data) {
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

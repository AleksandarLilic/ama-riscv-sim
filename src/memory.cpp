#include "memory.h"

memory::memory(
    std::string test_elf,
    cfg_t cfg,
    [[maybe_unused]] hw_cfg_t hw_cfg) :
        // create devices
        mm(MEM_SIZE, test_elf, hw_cfg),
        #ifdef UART_EN
        uart0(cfg),
        #endif
        clint0(),
        // put devices in memory map
        mem_map {{
            {BASE_ADDR, MEM_SIZE, &mm},
            #ifdef UART_EN
            {UART0_ADDR, UART_SIZE, &uart0},
            #endif
            {CLINT_ADDR, CLINT_SIZE, &clint0}
        }}
 {
    dev_ptr = nullptr;
 }

uint32_t memory::set_addr(uint32_t address, mem_op_t mem_op, uint32_t size) {
    bool dev_set = false;
    if (address < BASE_ADDR) {
        tu->e_dmem_access_fault(address, "below base address", mem_op);
        return 0;
    }

    for (auto &entry : mem_map) {
        if (entry.base <= address && address < entry.base + entry.size) {
            dev_ptr = entry.ptr;
            address -= entry.base;
            dev_set = true;
            break;
        }
    }

    if (!dev_set) {
        tu->e_dmem_access_fault(
            address, "undefined region or above max address", mem_op);
        return 0;
    }

    bool address_unaligned = (address % size != 0);
    if (address_unaligned) {
        tu->e_dmem_addr_misaligned(address, "unaligned access", mem_op);
        return 0;
    }

    return address;
}

scp_status_t memory::cache_hint(uint32_t address, scp_mode_t scp_mode) {
    #ifdef HW_MODELS_EN
    address = address - mem_map[0].base;
    dev_ptr = mem_map[0].ptr;
    main_memory* mm_ptr = static_cast<main_memory*>(this->dev_ptr);
    return mm_ptr->scp(address, scp_mode);
    #else
    // hint ignored if caches are not enabled
    SIM_WARNING << "Caches are not simulated but encoutered cache hint "
                << "at 0x" << std::hex << address << std::dec
                << " with mode " << TO_U32(scp_mode) << std::endl;
    return scp_status_t::fail;
    #endif
}

uint32_t memory::rd_inst(uint32_t address) {
    address = address - mem_map[0].base;
    bool address_unaligned = (address % 2 != 0);
    if (address_unaligned) {
        tu->e_inst_addr_misaligned(address, "unaligned access");
        return 0;
    }
    dev_ptr = mem_map[0].ptr;
    main_memory* mm_ptr = static_cast<main_memory*>(this->dev_ptr);
    return TO_U32(mm_ptr->rd_inst(address));
    //uint32_t inst = 0;
    //try {
    //    inst = TO_U32(mm_ptr->rd_inst(address));
    //} catch (const std::exception& e) {
    //    tu->e_inst_access_fault(address, e.what());
    //}
    //return inst;
}

uint32_t memory::just_inst(uint32_t address) {
    address = address - mem_map[0].base;
    dev_ptr = mem_map[0].ptr;
    main_memory* mm_ptr = static_cast<main_memory*>(this->dev_ptr);
    return TO_U32(mm_ptr->just_inst(address));
}

uint32_t memory::rd(uint32_t address, uint32_t size) {
    address = set_addr(address, mem_op_t::read, size);
    if (tu->is_trapped()) return 0;
    return dev_ptr->rd(address, size);
}

void memory::wr(uint32_t address, uint32_t data, uint32_t size) {
    address = set_addr(address, mem_op_t::write, size);
    if (tu->is_trapped()) return;
    dev_ptr->wr(address, data, size);
}

// xxd style byte dump
void memory::dump_as_bytes(uint32_t start, uint32_t size) {
    constexpr uint32_t bytes_per_row = 16;
    constexpr uint32_t word_boundary = 4;

    uint32_t aligned_start = start - (start % bytes_per_row);
    uint32_t offset = start - aligned_start; // alignment shifts it left
    uint32_t addr;

    for (uint32_t i = aligned_start; i < aligned_start + size + offset; i++) {
        if (i % bytes_per_row == 0) {
            // insert address at the start of each row
            std::cout << std::endl << MEM_ADDR_FORMAT(i) << ": ";
        }
        addr = set_addr(i, mem_op_t::read, 1u);
        std::cout << std::hex << std::right << std::setw(2) << std::setfill('0')
                  << dev_ptr->rd(addr, 1u) << " ";
        if (i % word_boundary == 3) std::cout << "  ";
    }
    std::cout << std::dec << std::left << std::endl;
}

// word per line
void memory::dump_as_words(uint32_t start, uint32_t size, std::string out_dir){
    constexpr uint32_t word_bytes = 4;
    uint32_t end = start + size;
    std::ofstream ofs;
    ofs.open(out_dir + "mem_dump.log");
    ofs << std::hex << "0x" << start << std::dec << ": " << size << " B\n";

    // process full 4-byte words
    for (uint32_t addr = start; addr + word_bytes <= end; addr += word_bytes) {
        uint32_t word = 0;

        // ready and pack bytes it into word as big-endian
        for (uint32_t i = 0; i < word_bytes; ++i) {
            uint32_t phys = set_addr(addr + i, mem_op_t::read, 1u);
            uint8_t b = dev_ptr->rd(phys, 1u);
            word |= static_cast<uint32_t>(b) << (8 * (i));
        }
        ofs << std::hex << std::setw(8) << std::setfill('0') << word << "\n";
    }
    std::cout << std::dec;
}

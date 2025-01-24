#include "memory.h"

memory::memory(
    std::string test_elf,
    [[maybe_unused]] hw_cfg_t hw_cfg) :
        // create devices
        mm(MEM_SIZE, test_elf, hw_cfg),
        #ifdef UART_ENABLE
        uart0(UART_SIZE),
        #endif
        // put devices in memory map
        mem_map {{
            {BASE_ADDR, MEM_SIZE, &mm},
            #ifdef UART_ENABLE
            {BASE_ADDR + MEM_SIZE, UART_SIZE, &uart0}
            #endif
        }}
 {
    dev_ptr = nullptr;
 }

uint32_t memory::set_addr(uint32_t address, access_t access, uint32_t size) {
    bool dev_set = false;
    if (address < BASE_ADDR) {
        tu->e_dmem_access_fault(address, "below base address", access);
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
            address, "undefined region or above max address", access);
        return 0;
    }

    bool address_unaligned = (address % size != 0);
    if (address_unaligned) {
        tu->e_dmem_addr_misaligned(address, "unaligned access", access);
        return 0;
    }

    return address;
}

scp_status_t memory::cache_hint(uint32_t address, scp_mode_t scp_mode) {
    #ifdef ENABLE_HW_PROF
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

uint32_t memory::rd(uint32_t address, uint32_t size) {
    address = set_addr(address, access_t::read, size);
    if (tu->is_trapped()) return 0;
    return dev_ptr->rd(address, size);
}

void memory::wr(uint32_t address, uint32_t data, uint32_t size) {
    address = set_addr(address, access_t::write, size);
    if (tu->is_trapped()) return;
    dev_ptr->wr(address, data, size);
}

// Dump (private)
void memory::mem_dump(uint32_t start, uint32_t size) {
    constexpr uint32_t bytes_per_row = 16;
    constexpr uint32_t word_boundary = 4;

    uint32_t aligned_start = start - (start % bytes_per_row);
    uint32_t offset = start - aligned_start; // alignment shifts it left
    uint32_t addr;

    for (uint32_t i = aligned_start; i < aligned_start + size + offset; i++) {
        if (i % bytes_per_row == 0) // insert address at the start of each row
            std::cout << std::endl << MEM_ADDR_FORMAT(i) << ": ";
        addr = set_addr(i, access_t::read, 1u);
        std::cout << std::right << std::setw(2) << std::setfill('0')
                  << dev_ptr->rd(addr, 1u) << " ";
        if (i % word_boundary == 3)
            std::cout << "  ";
    }
    std::cout << std::dec << std::left << std::endl;
}

// Dump entire memory to console
void memory::dump() {
    mem_dump(BASE_ADDR, MEM_SIZE);
}

// Dump memory range to console
void memory::dump(uint32_t start, uint32_t size) {
    mem_dump(start, size);
}

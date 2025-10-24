#include "clint.h"

clint::clint() : dev(CLINT_SIZE) {
    std::fill(mem.begin(), mem.end(), 0x0);
}

void clint::set_mip(uint32_t* csr_mip) {
    this->csr_mip = csr_mip;
}

uint64_t clint::get_mtime_shadow() {
    return dev::rd(MTIME, 8);
}

void clint::update_mtime(uint64_t mtime_elapsed) {
    // time in us, assume 10ns period (100MHz clck)
    // => 100 inst per us, as mtime counts insts in isa sim
    uint64_t mtime_prev = dev::rd(MTIME, 8);
    mtime_ticks += mtime_elapsed;
    dev::wr(MTIME, mtime_prev + (mtime_ticks / 100), 8);
    mtime_ticks = mtime_ticks % 100; // for next go around

    if (dev::rd(MTIME, 8) >= dev::rd(MTIMECMP, 8)) *csr_mip |= MIP_MTIP;
    else *csr_mip &= ~MIP_MTIP;
}

void clint::update_mtime() {
    update_mtime(1);
}

uint32_t clint::rd(uint32_t address, uint32_t size) {
    if ((address >= MTIMECMP) && (address < MTIME + 8)) {
        return dev::rd(address, size);
    } else {
        tu->e_dmem_access_fault(address, "clint: read error", mem_op_t::read);
        return 0;
    }
}

void clint::wr(uint32_t address, uint32_t data, uint32_t size) {
    if ((address >= MTIMECMP) && (address < MTIME + 8)) {
        dev::wr(address, data, size);
    } else {
        tu->e_dmem_access_fault(address, "clint: write error", mem_op_t::write);
    }
}

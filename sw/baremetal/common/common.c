#include "common.h"

// uart
void send_byte_uart0(char byte) {
    while (!UART0_TX_READY);
    UART0->tx_data = byte;
}

int _write(int fd, char* ptr, int len) {
    (void)fd;
    int count = len;
    while (count-- > 0) {
        send_byte_uart0(*ptr);
        ptr++;
    }
    return len;
}

int __puts_uart(char *s, int len, void* buf) {
    (void)buf;
    _write(0, s, len);
    return len;
}

int mini_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int count = mini_vpprintf(__puts_uart, NULL, format, args);
    va_end(args);
    return count;
}

void set_cpu_time(uint64_t value) {
    write_csr_wide(CSR_TIME, CSR_TIMEH, value);
}

// cpu perf counters
uint64_t get_cpu_time() {
    uint64_t time;
    read_csr_wide(CSR_TIME, CSR_TIMEH, time);
    return time;
}

void set_cpu_cycles(uint64_t value) {
    write_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, value);
}

uint64_t get_cpu_cycles() {
    uint64_t cycles;
    read_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, cycles);
    return cycles;
}

void set_cpu_instret(uint64_t value) {
    write_csr_wide(CSR_MINSTRET, CSR_MINSTRETH, value);
}

uint64_t get_cpu_instret() {
    uint64_t instret;
    read_csr_wide(CSR_MINSTRET, CSR_MINSTRETH, instret);
    return instret;
}

// tda
void init_tda_counters() {
    // reset existing event counters
    write_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, 0u);
    write_csr_wide(CSR_MHPMCOUNTER3, CSR_MHPMCOUNTER3H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER4, CSR_MHPMCOUNTER4H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER5, CSR_MHPMCOUNTER5H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER6, CSR_MHPMCOUNTER6H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER7, CSR_MHPMCOUNTER7H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER8, CSR_MHPMCOUNTER8H, 0u);
    // set up events L1 events first
    write_csr(CSR_MHPMEVENT3, mhpmevent_bad_spec);
    write_csr(CSR_MHPMEVENT4, mhpmevent_stall_fe);
    write_csr(CSR_MHPMEVENT5, mhpmevent_stall_be);
    // then L2 events
    write_csr(CSR_MHPMEVENT6, mhpmevent_stall_l1i);
    write_csr(CSR_MHPMEVENT7, mhpmevent_stall_l1d);
    write_csr(CSR_MHPMEVENT8, mhpmevent_ret_simd);
}

void save_tda_counters(tda_cnt_t* p) {
    // read L2 events first
    read_csr_wide(CSR_MHPMCOUNTER6, CSR_MHPMCOUNTER6H, p->fe_ic);
    read_csr_wide(CSR_MHPMCOUNTER7, CSR_MHPMCOUNTER7H, p->be_dc);
    read_csr_wide(CSR_MHPMCOUNTER8, CSR_MHPMCOUNTER8H, p->ret_simd);
    // then L1 events
    read_csr_wide(CSR_MHPMCOUNTER3, CSR_MHPMCOUNTER3H, p->bad_spec);
    read_csr_wide(CSR_MHPMCOUNTER4, CSR_MHPMCOUNTER4H, p->fe);
    read_csr_wide(CSR_MHPMCOUNTER5, CSR_MHPMCOUNTER5H, p->be);
    read_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, p->cycles);
    // figure out the rest
    p->be_core = (p->be - p->be_dc);
    p->fe_core = (p->fe - p->fe_ic);
    p->ret = (p->cycles - (p->bad_spec + p->fe + p->be));
}

void print_ipc(const uint64_t cycles, const uint64_t ret) {
    // scale up by 1000x to get 3 decimal places w/o floating point
    const uint32_t sf_ipc = 1000u;
    uint32_t ipc_x = ((ret * sf_ipc) / cycles);
    printf(
        "Stats:\n"
        INDENT "Cycles: %u, Retired: %u, Empty: %u, IPC: %u.%03u\n",
        (uint32_t)cycles,
        (uint32_t)ret,
        (uint32_t)(cycles - ret),
        (ipc_x / sf_ipc), (ipc_x % sf_ipc)
    );
}

void print_tda_counters(const tda_cnt_t* p) {
    print_ipc(p->cycles, p->ret);
    printf(
        "TDA:\n"
        INDENT "L1: "
        "Bad Spec: %u, FE: %u, BE: %u, Retired: %u\n"
        INDENT "L2: "
        "FE ICache: %u, FE Core: %u, "
        "BE DCache: %u, BE Core: %u, "
        "INT: %u, SIMD: %u\n\n",
        (uint32_t)p->bad_spec, (uint32_t)p->fe,(uint32_t)p->be,(uint32_t)p->ret,
        (uint32_t)p->fe_ic, (uint32_t)p->fe_core,
        (uint32_t)p->be_dc, (uint32_t)p->be_core,
        (uint32_t)p->ret, (uint32_t)p->ret_simd
    );
}

void print_tda_counters_json(const tda_cnt_t* p) {
    printf(
        "{\"bad_spec\": %u, \"stall_l1i\": %u, \"stall_fe_core\": %u, "
        "\"stall_l1d\": %u, \"stall_be_core\": %u, \"ret_int\": %u, "
        "\"ret_simd\": %u}\n\n",
        (uint32_t)p->bad_spec,
        (uint32_t)p->fe_ic,
        (uint32_t)p->fe_core,
        (uint32_t)p->be_dc,
        (uint32_t)p->be_core,
        (uint32_t)(p->ret - p->ret_simd),
        (uint32_t)p->ret_simd
    );
}

// baseline hw counters
void init_hw_counters() {
    // reset existing event counters
    write_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, 0u);
    write_csr_wide(CSR_MINSTRET, CSR_MINSTRETH, 0u);
    write_csr_wide(CSR_MHPMCOUNTER3, CSR_MHPMCOUNTER3H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER4, CSR_MHPMCOUNTER4H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER5, CSR_MHPMCOUNTER5H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER6, CSR_MHPMCOUNTER6H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER7, CSR_MHPMCOUNTER7H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER8, CSR_MHPMCOUNTER8H, 0u);
    // set up hw events
    write_csr(CSR_MHPMEVENT3, mhpmevent_ret_ctrl_flow_br);
    write_csr(CSR_MHPMEVENT4, mhpmevent_bp_miss);
    write_csr(CSR_MHPMEVENT5, mhpmevent_l1i_ref);
    write_csr(CSR_MHPMEVENT6, mhpmevent_l1i_miss);
    write_csr(CSR_MHPMEVENT7, mhpmevent_l1d_ref);
    write_csr(CSR_MHPMEVENT8, mhpmevent_l1d_miss);
}

void save_hw_counters(hw_cnt_t* p) {
    read_csr_wide(CSR_MHPMCOUNTER3, CSR_MHPMCOUNTER3H, p->ret_ctrl_flow_br);
    read_csr_wide(CSR_MHPMCOUNTER4, CSR_MHPMCOUNTER4H, p->bp_miss);
    read_csr_wide(CSR_MHPMCOUNTER5, CSR_MHPMCOUNTER5H, p->l1i_ref);
    read_csr_wide(CSR_MHPMCOUNTER6, CSR_MHPMCOUNTER6H, p->l1i_miss);
    read_csr_wide(CSR_MHPMCOUNTER7, CSR_MHPMCOUNTER7H, p->l1d_ref);
    read_csr_wide(CSR_MHPMCOUNTER8, CSR_MHPMCOUNTER8H, p->l1d_miss);
    read_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, p->cycles);
    read_csr_wide(CSR_MINSTRET, CSR_MINSTRETH, p->ret);
}

void print_hw_counters(const hw_cnt_t* p) {
    const uint32_t sf_hw = 100u;

    uint32_t bp_hr_x =
        ((sf_hw - ((p->bp_miss * sf_hw) / p->ret_ctrl_flow_br)) * 100u);
    uint32_t bp_mpki_x = ((p->bp_miss * sf_hw) / (p->ret / 1000));

    uint32_t l1i_hr_x = ((sf_hw - ((p->l1i_miss * sf_hw) / p->l1i_ref)) * 100u);
    uint32_t l1i_mpki_x = ((p->l1i_miss * sf_hw) / (p->ret / 1000));

    uint32_t l1d_hr_x = ((sf_hw - ((p->l1d_miss * sf_hw) / p->l1d_ref)) * 100u);
    uint32_t l1d_mpki_x = ((p->l1d_miss * sf_hw) / (p->ret / 1000));

    print_ipc(p->cycles, p->ret);

    uint32_t bp_hit = (p->ret_ctrl_flow_br - p->bp_miss);
    printf(
        "bpred:\n"
        INDENT "P: %u, M: %u, ACC: %u.%02u%%, MPKI: %u.%02u\n",
        (uint32_t)bp_hit, (uint32_t)p->bp_miss,
        (bp_hr_x / sf_hw), (bp_hr_x % sf_hw),
        (bp_mpki_x / sf_hw), (bp_mpki_x % sf_hw)
    );

    uint32_t l1i_hit = (p->l1i_ref - p->l1i_miss);
    printf(
        "icache:\n"
        INDENT "Ref: %u, H: %u, M: %u, HR: %u.%02u%%, MPKI: %u.%02u\n",
        (uint32_t)p->l1i_ref, (uint32_t)l1i_hit, (uint32_t)p->l1i_miss,
        (l1i_hr_x / sf_hw), (l1i_hr_x % sf_hw),
        (l1i_mpki_x / sf_hw), (l1i_mpki_x % sf_hw)
    );

    uint32_t l1d_hit = (p->l1d_ref - p->l1d_miss);
    printf(
        "dcache:\n"
        INDENT "Ref: %u, H: %u, M: %u, HR: %u.%02u%%, MPKI: %u.%02u\n",
        (uint32_t)p->l1d_ref, (uint32_t)l1d_hit, (uint32_t)p->l1d_miss,
        (l1d_hr_x / sf_hw), (l1d_hr_x % sf_hw),
        (l1d_mpki_x / sf_hw), (l1d_mpki_x % sf_hw)
    );
    printf("\n");
}

void print_hw_counters_json(const hw_cnt_t* p) {
    printf(
        "{\"cycles\": %u, \"ret\": %u, "
        "\"ret_ctrl_flow_br\": %u, \"bp_miss\": %u, "
        "\"l1i_ref\": %u, \"l1i_miss\": %u, "
        "\"l1d_ref\": %u, \"l1d_miss\": %u}\n\n",
        (uint32_t)p->cycles,
        (uint32_t)p->ret,
        (uint32_t)p->ret_ctrl_flow_br,
        (uint32_t)p->bp_miss,
        (uint32_t)p->l1i_ref,
        (uint32_t)p->l1i_miss,
        (uint32_t)p->l1d_ref,
        (uint32_t)p->l1d_miss
    );
}

// traps
void __attribute__((weak))
trap_handler(unsigned int mcause, void* mepc, void* sp) {
    (void)mepc;
    (void)sp;

    if (mcause < 0x80000000) { // can't handle exceptions, just exit
        write_mismatch(0, 0, 1000 + mcause);
        fail();
    } else { // interrupts
        if (mcause == MCAUSE_MACHINE_TIMER_INT) {
            timer_interrupt_handler();
            return;
        }
        // other unsupported atm
        write_mismatch(0, 0, 2000 + mcause);
        fail();
    }
    return;
}

void __attribute__((weak))
timer_interrupt_handler() {
    CLINT->mtimecmp += 10000; // 10ms slices
}

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

static void npf_putc_uart(int c, void *ctx) {
    (void)ctx;
    send_byte_uart0((char)c);
}

int npf_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int count = npf_vpprintf(npf_putc_uart, NULL, format, args);
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
    write_csr_wide(CSR_MHPMCOUNTER3, CSR_MHPMCOUNTER3H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER4, CSR_MHPMCOUNTER4H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER5, CSR_MHPMCOUNTER5H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER6, CSR_MHPMCOUNTER6H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER7, CSR_MHPMCOUNTER7H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER8, CSR_MHPMCOUNTER8H, 0u);
    write_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, 0u);
    // set up events L1 events first
    write_csr(CSR_MHPMEVENT4, mhpmevent_stall_fe);
    write_csr(CSR_MHPMEVENT5, mhpmevent_stall_be);
    // then L2 events
    write_csr(CSR_MHPMEVENT3, mhpmevent_bad_spec);
    write_csr(CSR_MHPMEVENT6, mhpmevent_stall_l1i);
    write_csr(CSR_MHPMEVENT7, mhpmevent_stall_l1d);
    write_csr(CSR_MHPMEVENT8, mhpmevent_ret_simd);
    write_csr_wide(CSR_MINSTRET, CSR_MINSTRETH, 0u);
}

void save_tda_counters(tda_cnt_t* p) {
    // read L2 events first
    read_csr_wide(CSR_MINSTRET, CSR_MINSTRETH, p->ret);
    read_csr_wide(CSR_MHPMCOUNTER3, CSR_MHPMCOUNTER3H, p->bad_spec);
    read_csr_wide(CSR_MHPMCOUNTER6, CSR_MHPMCOUNTER6H, p->stall_l1i);
    read_csr_wide(CSR_MHPMCOUNTER7, CSR_MHPMCOUNTER7H, p->stall_l1d);
    read_csr_wide(CSR_MHPMCOUNTER8, CSR_MHPMCOUNTER8H, p->ret_simd);
    // then L1 events
    read_csr_wide(CSR_MHPMCOUNTER4, CSR_MHPMCOUNTER4H, p->stall_fe);
    read_csr_wide(CSR_MHPMCOUNTER5, CSR_MHPMCOUNTER5H, p->stall_be);
    read_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, p->cycles);
    // derive the rest
    p->empty = (p->cycles - p->ret);
    p->stalls = (p->stall_be + p->stall_fe);
    p->lost = (p->empty - p->stalls);
    p->lost_other = (p->lost - p->bad_spec);
    p->stall_be_core = (p->stall_be - p->stall_l1d);
    p->stall_fe_core = (p->stall_fe - p->stall_l1i);
    p->ret_int = (p->ret - p->ret_simd);
}

void print_ipc(const uint64_t cycles, const uint64_t ret) {
    // scale up by 1000x to get 3 decimal places w/o floating point
    const uint32_t sf_ipc = 1000u;
    uint32_t ipc_x = ((ret * sf_ipc) / cycles);
    printf(
        "Stats:\n"
        INDENT "Cycles: %llu, Retired: %llu, Empty: %llu, IPC: %u.%03u\n",
        cycles, ret, (cycles - ret), (ipc_x / sf_ipc), (ipc_x % sf_ipc)
    );
}

void print_tda_counters(const tda_cnt_t* p) {
    print_ipc(p->cycles, p->ret);
    printf(
        "TDA:\n"
        INDENT "L1: Retired: %llu, FE: %llu, BE: %llu, Lost: %llu\n",
        p->ret, p->stall_fe, p->stall_be, p->lost
    );
    printf(
        INDENT "L2: "
        "INT: %llu, SIMD: %llu, "
        "FE ICache: %llu, FE Core: %llu, "
        "BE DCache: %llu, BE Core: %llu, "
        "Bad Spec: %llu, Other: %llu"
        "\n\n",
        p->ret, p->ret_simd,
        p->stall_l1i, p->stall_fe_core,
        p->stall_l1d, p->stall_be_core,
        p->bad_spec, p->lost_other
    );
}

void print_tda_counters_json(const tda_cnt_t* p) {
    printf(
        "{"
        "\"cycles\": %llu, "
        "\"empty\": %llu, "
        "\"stalls\": %llu, "
        "\"lost\": %llu, "
        "\"lost_other\": %llu, "
        "\"bad_spec\": %llu, "
        "\"stall_be\": %llu, "
        "\"stall_l1d\": %llu, "
        "\"stall_be_core\": %llu, "
        "\"stall_fe\": %llu, "
        "\"stall_l1i\": %llu, "
        "\"stall_fe_core\": %llu, "
        "\"ret\": %llu, "
        "\"ret_simd\": %llu, "
        "\"ret_int\": %llu"
        "}\n\n",
        p->cycles,
        p->empty,
        p->stalls,
        p->lost,
        p->lost_other,
        p->bad_spec,
        p->stall_be,
        p->stall_l1d,
        p->stall_be_core,
        p->stall_fe,
        p->stall_l1i,
        p->stall_fe_core,
        p->ret,
        p->ret_simd,
        p->ret_int
    );
}

// baseline hw counters
void init_hw_counters() {
    // reset existing event counters
    write_csr_wide(CSR_MHPMCOUNTER3, CSR_MHPMCOUNTER3H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER4, CSR_MHPMCOUNTER4H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER5, CSR_MHPMCOUNTER5H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER6, CSR_MHPMCOUNTER6H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER7, CSR_MHPMCOUNTER7H, 0u);
    write_csr_wide(CSR_MHPMCOUNTER8, CSR_MHPMCOUNTER8H, 0u);
    // set up hw events
    write_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, 0u);
    write_csr(CSR_MHPMEVENT3, mhpmevent_ret_ctrl_flow_br);
    write_csr(CSR_MHPMEVENT4, mhpmevent_bp_miss);
    write_csr(CSR_MHPMEVENT5, mhpmevent_l1i_ref);
    write_csr(CSR_MHPMEVENT6, mhpmevent_l1i_miss);
    write_csr(CSR_MHPMEVENT7, mhpmevent_l1d_ref);
    write_csr(CSR_MHPMEVENT8, mhpmevent_l1d_miss);
    write_csr_wide(CSR_MINSTRET, CSR_MINSTRETH, 0u);
}

void save_hw_counters(hw_cnt_t* p) {
    read_csr_wide(CSR_MINSTRET, CSR_MINSTRETH, p->ret);
    read_csr_wide(CSR_MHPMCOUNTER3, CSR_MHPMCOUNTER3H, p->ret_ctrl_flow_br);
    read_csr_wide(CSR_MHPMCOUNTER4, CSR_MHPMCOUNTER4H, p->bp_miss);
    read_csr_wide(CSR_MHPMCOUNTER5, CSR_MHPMCOUNTER5H, p->l1i_ref);
    read_csr_wide(CSR_MHPMCOUNTER6, CSR_MHPMCOUNTER6H, p->l1i_miss);
    read_csr_wide(CSR_MHPMCOUNTER7, CSR_MHPMCOUNTER7H, p->l1d_ref);
    read_csr_wide(CSR_MHPMCOUNTER8, CSR_MHPMCOUNTER8H, p->l1d_miss);
    read_csr_wide(CSR_MCYCLE, CSR_MCYCLEH, p->cycles);
}

static uint32_t calc_hr(uint64_t ref, uint64_t miss, uint64_t decimal_scale) {
    const uint64_t percent_scale = 100ull;

    if (ref == 0u) return 0u;
    uint64_t hit = (miss < ref) ? (ref - miss) : 0u;
    return (uint32_t)((hit * percent_scale * decimal_scale) / ref);
}

static uint32_t calc_mpki(uint64_t miss, uint64_t ret, uint64_t decimal_scale) {
    const uint64_t per_kilo_scale = 1000ull;

    if (ret == 0u) return 0u;
    return (uint32_t)((miss * per_kilo_scale * decimal_scale) / ret);
}

void print_hw_counters(const hw_cnt_t* p) {
    const uint32_t print_sc = 100u;
    const uint64_t dec_sc = (uint64_t)print_sc;

    uint32_t bp_hr_x = calc_hr(p->ret_ctrl_flow_br, p->bp_miss, dec_sc);
    uint32_t bp_mpki_x = calc_mpki(p->bp_miss, p->ret, dec_sc);

    uint32_t l1i_hr_x = calc_hr(p->l1i_ref, p->l1i_miss, dec_sc);
    uint32_t l1i_mpki_x = calc_mpki(p->l1i_miss, p->ret, dec_sc);

    uint32_t l1d_hr_x = calc_hr(p->l1d_ref, p->l1d_miss, dec_sc);
    uint32_t l1d_mpki_x = calc_mpki(p->l1d_miss, p->ret, dec_sc);

    print_ipc(p->cycles, p->ret);

    uint64_t bp_hit = (p->bp_miss < p->ret_ctrl_flow_br) ?
        (p->ret_ctrl_flow_br - p->bp_miss) : 0u;
    printf(
        "bpred:\n"
        INDENT "P: %llu, M: %llu, ACC: %u.%02u%%, MPKI: %u.%02u\n",
        bp_hit, p->bp_miss,
        (bp_hr_x / print_sc), (bp_hr_x % print_sc),
        (bp_mpki_x / print_sc), (bp_mpki_x % print_sc)
    );

    uint64_t l1i_hit = (p->l1i_miss < p->l1i_ref) ?
        (p->l1i_ref - p->l1i_miss) : 0u;
    printf(
        "icache:\n"
        INDENT "Ref: %llu, H: %llu, M: %llu, HR: %u.%02u%%, MPKI: %u.%02u\n",
        p->l1i_ref, l1i_hit, p->l1i_miss,
        (l1i_hr_x / print_sc), (l1i_hr_x % print_sc),
        (l1i_mpki_x / print_sc), (l1i_mpki_x % print_sc)
    );

    uint64_t l1d_hit = (p->l1d_miss < p->l1d_ref) ?
        (p->l1d_ref - p->l1d_miss) : 0u;
    printf(
        "dcache:\n"
        INDENT "Ref: %llu, H: %llu, M: %llu, HR: %u.%02u%%, MPKI: %u.%02u\n",
        p->l1d_ref, l1d_hit, p->l1d_miss,
        (l1d_hr_x / print_sc), (l1d_hr_x % print_sc),
        (l1d_mpki_x / print_sc), (l1d_mpki_x % print_sc)
    );
    printf("\n");
}

void print_hw_counters_json(const hw_cnt_t* p) {
    printf(
        "{"
        "\"cycles\": %llu, "
        "\"ret\": %llu, "
        "\"ret_ctrl_flow_br\": %llu, "
        "\"bp_miss\": %llu, "
        "\"l1i_ref\": %llu, "
        "\"l1i_miss\": %llu, "
        "\"l1d_ref\": %llu, "
        "\"l1d_miss\": %llu"
        "}\n\n",
        p->cycles,
        p->ret,
        p->ret_ctrl_flow_br,
        p->bp_miss,
        p->l1i_ref,
        p->l1i_miss,
        p->l1d_ref,
        p->l1d_miss
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

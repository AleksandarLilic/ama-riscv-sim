#include "uart.h"

#ifndef DPI
#ifdef UART_INPUT_EN
#include <poll.h>
#include <unistd.h>
#endif
#endif

uart::uart(cfg_t cfg) :
    dev(UART_SIZE),
    sink_uart(cfg.sink_uart)
    #ifndef DPI
    #ifdef UART_INPUT_EN
    , uart_in(cfg.uart_in)
    , uart_in_idx(0)
    , next_rx_inst(BAUD_STRIDE)
    , csr_mip(nullptr)
    #endif
    #endif
{
    std::fill(mem.begin(), mem.end(), 0);
    mem[UART_STATUS] |= UART_TX_READY; // always ready to transmit
    uart_ofs.open(cfg.out_dir + "uart.log");
}

uart::~uart() {}

#ifdef DPI
// force flush to file on every character, unreliable output otherwise
#define UART_FLUSH << std::flush;
#else
#define UART_FLUSH
#endif

void uart::wr(uint32_t address, uint32_t data, uint32_t size) {
    // writes to status and rx_data registers are ignored
    if (address == UART_TX_DATA) {
        // write to memory
        dev::wr(address, TO_U8(data), size);
        // emulate the effect of writing to uart tx_data register
        uart_ofs << TO_U8(data) UART_FLUSH;
        if (!sink_uart) std::cout << TO_U8(data) << std::flush;
    }
}

#ifndef DPI
#ifdef UART_INPUT_EN
uint32_t uart::rd(uint32_t address, uint32_t size) {
    // reads from status register
    if (address < UART_RX_DATA) return dev::rd(address, size);
    // reads from rx_data register consuming the byte clears RX_VALID (and MEIP)
    if (address == UART_RX_DATA) {
        mem[UART_STATUS] &= ~UART_RX_VALID;
        refresh_meip();
        return dev::rd(address, size);
    }
    return 0; // tx data not readable, return 0
}

void uart::refresh_meip() {
    if (csr_mip == nullptr) return;
    if (mem[UART_STATUS] & UART_RX_VALID) *csr_mip |= MIP_MEIP;
    else *csr_mip &= ~MIP_MEIP;
}

int32_t uart::next_byte() {
    if (!uart_in.empty()) {
        // preloaded source: deterministic, exhausts at end of string
        if (uart_in_idx < uart_in.size())
            return TO_U8(uart_in[uart_in_idx++]);
        return -1;
    }
    // live source: non-blocking stdin poll
    struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
    if (::poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
        char c;
        if (::read(STDIN_FILENO, &c, 1) == 1) return TO_U8(c);
    }
    return -1; // nothing available (or EOF)
}

void uart::update_input(uint64_t instr_cnt) {
    #ifndef DPI
    if (instr_cnt >= next_rx_inst) {
        next_rx_inst += BAUD_STRIDE;
        // only pull a new byte once the previous one is consumed,
        // so MEIP stays a clean level and no source byte is dropped
        if (!(mem[UART_STATUS] & UART_RX_VALID)) {
            int32_t b = next_byte();
            if (b >= 0) {
                mem[UART_RX_DATA] = TO_U8(b);
                mem[UART_STATUS] |= UART_RX_VALID;
            }
        }
    }
    #else
    (void)instr_cnt;
    #endif
    refresh_meip();
}
#endif // UART_INPUT_EN
#endif // DPI

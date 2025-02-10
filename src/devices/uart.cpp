#include "uart.h"

uart::uart() : dev(UART_SIZE) {
    std::fill(mem.begin(), mem.end(), 0);
    mem[UART_STATUS] |= UART_TX_READY; // always ready to transmit
    #ifdef UART_INPUT_EN
    uart_running = true;
    uart_in_ready = false;
    uart_thread = std::thread(&uart::uart_stdin, this, uart_baud_rate::_115200);
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] () { return uart_in_ready.load(); });
    #endif
}

uart::~uart() {
    #ifdef UART_INPUT_EN
    uart_running = false;
    if (uart_thread.joinable()) uart_thread.join();
    #endif
}

void uart::wr(uint32_t address, uint32_t data, uint32_t size) {
    // writes to status and rx_data registers are ignored
    if (address == UART_TX_DATA) {
        // write to memory
        dev::wr(address, TO_U8(data), size);
        // emulate the effect of writing to uart tx_data register
        std::cout << TO_U8(data) << std::flush;
    }
}

#ifdef UART_INPUT_EN
uint32_t uart::rd(uint32_t address, uint32_t size) {
    // reads from status register
    if (address < UART_RX_DATA) return dev::rd(address, size);
    // reads from rx_data register with lock
    if (address == UART_RX_DATA) {
        std::lock_guard<std::mutex> lock(mtx);
        mem[UART_STATUS] &= ~UART_RX_VALID;
        return dev::rd(address, size);
    }
    return 0; // tx data not readable, return 0
}

void uart::uart_stdin(uart_baud_rate baud_rate) {
    // assuming 10 bits per character for 8N1
    auto delay = std::chrono::microseconds(1'000'000 * 10 / TO_I32(baud_rate));
    {
        std::lock_guard<std::mutex> lock(mtx);
        uart_in_ready = true;
    }
    cv.notify_one();

    std::string input_buffer;
    char c;
    while (uart_running) {
        c = std::getchar();
        if (c == '\n') {
            for (char c : input_buffer) {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    mem[UART_RX_DATA] = c;
                    mem[UART_STATUS] |= UART_RX_VALID;
                }
                std::this_thread::sleep_for(delay);
            }
            input_buffer.clear();
        } else {
            input_buffer.push_back(c);
        }
    }
}
#endif

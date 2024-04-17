#include "uart.h"

uart::uart(size_t size) :
    dev(size)
{
    std::fill(mem.begin(), mem.end(), 0);
    mem[UART_STATUS] |= UART_TX_READY; // always ready to transmit
    #ifdef UART_INPUT_ENABLED
    uart_running = true;
    uart_in_ready = false;
    uart_thread = std::thread(&uart::uart_stdin, this, 
                              uart_baud_rate::baud_115200);
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] () { return uart_in_ready.load(); });
    #endif
}

uart::~uart() {
    #ifdef UART_INPUT_ENABLED
    uart_running = false;
    if(uart_thread.joinable())
        uart_thread.join();
    #endif
}

inline void uart::wr(uint32_t address, uint8_t data) {
    // writes to status and rx_data registers are ignored
    if (address == UART_TX_DATA) {
        mem[address] = data;
        std::cout << data << std::flush;
    }
}

inline uint8_t uart::rd(uint32_t address) {
    if (address < UART_RX_DATA) // reads from status register
        return mem[address];
    
    if (address == UART_RX_DATA) { // reads from rx_data register with lock
        #ifdef UART_INPUT_ENABLED
        std::lock_guard<std::mutex> lock(mtx);
        mem[UART_STATUS] &= ~UART_RX_VALID;
        #endif
        return mem[UART_RX_DATA];
    }
    return 0; // tx data not readable, return 0
}

#ifdef UART_INPUT_ENABLED
void uart::uart_stdin(uart_baud_rate baud_rate) {
    // assuming 10 bits per character for 8N1
    auto delay = std::chrono::microseconds(1'000'000 * 10 / (int)baud_rate);
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

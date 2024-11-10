#pragma once

#include "defines.h"
#include "dev.h"

#ifdef UART_INPUT_ENABLE
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>

enum class uart_baud_rate {
    _9600 = 9600,
    _19200 = 19200,
    _38400 = 38400,
    _57600 = 57600,
    _115200 = 115200,
    _230400 = 230400,
    _460800 = 460800
};
#endif

class uart : public dev {
    private:
        static const uint8_t UART_TX_READY = 0x01;
        static const uint8_t UART_RX_VALID = 0x02;
        static const uint8_t UART_STATUS = 0x00;
        static const uint8_t UART_RX_DATA = 0x04;
        static const uint8_t UART_TX_DATA = 0x08;
        #ifdef UART_INPUT_ENABLE
        std::mutex mtx;
        std::thread uart_thread;
        std::atomic<bool> uart_running;
        std::atomic<bool> uart_in_ready;
        std::condition_variable cv;
        #endif
    public:
        uart(size_t size);
        ~uart();
        void wr(uint32_t address, uint32_t data, uint32_t size) override;
    #ifdef UART_INPUT_ENABLE
        uint32_t rd(uint32_t address, uint32_t size) override;
    private:
        void uart_stdin(uart_baud_rate baud_rate);
    #endif
};

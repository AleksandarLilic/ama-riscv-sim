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
    baud_9600 = 9600,
    baud_19200 = 19200,
    baud_38400 = 38400,
    baud_57600 = 57600,
    baud_115200 = 115200,
    baud_230400 = 230400,
    baud_460800 = 460800
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
        void wr(uint32_t address, uint8_t data);
    #ifdef UART_INPUT_ENABLE
        uint8_t rd(uint32_t address);
    private:
        void uart_stdin(uart_baud_rate baud_rate);
    #endif
};

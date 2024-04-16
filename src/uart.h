#pragma once

#include "defines.h"
#include "dev.h"

#ifdef UART_INPUT_ENABLED
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
#endif

class uart : public dev {
    private:
        static const uint8_t UART_TX_READY = 0x01;
        static const uint8_t UART_RX_VALID = 0x02;
        static const uint8_t UART_STATUS = 0x00;
        static const uint8_t UART_RX_DATA = 0x04;
        static const uint8_t UART_TX_DATA = 0x08;
        #ifdef UART_INPUT_ENABLED
        std::mutex mtx;
        std::thread uart_thread;
        std::atomic<bool> uart_running;
        std::atomic<bool> uart_in_ready;
        std::condition_variable cv;
        #endif
    public:
        uart(size_t size);
        ~uart();
        uint8_t rd(uint32_t address);
        void wr(uint32_t address, uint8_t data);
    #ifdef UART_INPUT_ENABLED
    private:
        void uart_stdin(uint32_t baud_rate = 9600);
    #endif
};
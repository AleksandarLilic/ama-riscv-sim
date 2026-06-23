#pragma once

#include "defines.h"
#include "dev.h"

enum class uart_baud_rate {
    _9600 = 9600,
    _19200 = 19200,
    _38400 = 38400,
    _57600 = 57600,
    _115200 = 115200,
    _230400 = 230400,
    _460800 = 460800,
    _576000 = 576000,
    _921600 = 921600
};

class uart : public dev {
    private:
        static const uint8_t UART_TX_READY = 0x01;
        static const uint8_t UART_RX_VALID = 0x02;
        static const uint8_t UART_STATUS = 0x00;
        static const uint8_t UART_RX_DATA = 0x04;
        static const uint8_t UART_TX_DATA = 0x08;
        std::ofstream uart_ofs;
        const bool sink_uart;

        #ifndef DPI
        #ifdef UART_INPUT_EN
        // RX input is drained one byte per ~char-time
        // on the same virtual clock as mtime (10ns/inst @ 100MHz, 1 IPC):
        // instr/char = (10 bits / baud) / 10ns = 1e9 / baud
        // 8N1 => 10 bits/char
        static const uart_baud_rate baud_rate = uart_baud_rate::_115200;
        static const uint64_t BAUD_STRIDE =
            (1'000'000'000ULL / static_cast<uint64_t>(baud_rate));
        // RX source: preloaded byte queue; if empty, poll stdin live
        const std::string uart_in;
        size_t uart_in_idx;
        uint64_t next_rx_inst;
        uint32_t* csr_mip;
        // next RX byte from the active source; 1 if none available
        int32_t next_byte();
        void refresh_meip(); // drive mip.MEIP from RX_VALID (level-sensitive)
        #endif
        #endif

    public:
        uart(cfg_t cfg);
        ~uart();
        void wr(uint32_t address, uint32_t data, uint32_t size) override;
        #ifndef DPI
        #ifdef UART_INPUT_EN
        uint32_t rd(uint32_t address, uint32_t size) override;
        void set_mip(uint32_t* csr_mip) { this->csr_mip = csr_mip; }
        void update_input(uint64_t instr_cnt); // paced RX drain
        #endif
        #endif
};

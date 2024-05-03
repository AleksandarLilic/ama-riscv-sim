#define UART_REG_CTRL ((volatile unsigned int*) 0x80004000)
#define UART_REG_RX_DATA ((volatile unsigned int*) 0x80004004)
#define UART_REG_TX_DATA ((volatile unsigned int*) 0x80004008)

#define UART_TX_READY_MASK 0b01
#define UART_RX_VALID_MASK 0b10
#define UART_TX_READY (*UART_REG_CTRL & UART_TX_READY_MASK)
#define UART_RX_VALID (*UART_REG_CTRL & UART_RX_VALID_MASK)

#define UART_RX_DATA (*UART_REG_RX_DATA)
#define UART_TX_DATA (*UART_REG_TX_DATA)

void main(void) {
    int offset = 0x20;
    for (char i = 0; i < 10; i++) {
        while (!UART_RX_VALID);
        char received_byte = UART_RX_DATA;
        while (!UART_TX_READY);
        UART_TX_DATA = received_byte - offset;
    }
    
    asm volatile("ecall");
}

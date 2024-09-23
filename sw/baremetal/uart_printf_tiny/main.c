#include <stdint.h>

#include "common.h"
#include "mem_map.h"
#include "printf.h"

void main(void) {
    while (!UART0_TX_READY);
    for (int j = 0; j < 2; j++) {
        printf("Hello, world!\n");
    }
    pass();
}

#include <stdint.h>
#include "common.h"
#include "mem_map.h"

void main(void) {
    const char msg[] = "Hello, world!\n";
    for (int j = 0; j < 2; j++)
        write_uart0(0, (char *)msg, sizeof(msg));
    pass();
}

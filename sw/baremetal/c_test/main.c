#define LOOP_COUNT 100000

void fail();

const unsigned int ref[16] = {116, 131, 144, 155, 164, 171, 176, 179, 180, 179, 176, 171, 164, 155, 144, 131};

volatile unsigned int a[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
volatile unsigned int b[16] = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
volatile unsigned int c[16];

int asm_add(unsigned int a, unsigned int b) {
    asm volatile("add %0, %1, %2" 
                 : "=r"(a)
                 : "r"(a), "r"(b));
    return a;
}

void set_c() {
    for (int i = 0; i < 16; i++) {
        c[i] = 100 + i;
    }
}

int main(void) {
    for (int i = 0; i < LOOP_COUNT; i++) {
        set_c();
        
        for (int j = 0; j < 16; j++) {
            c[j] = asm_add(c[j], a[j] * b[j]);
        }

        for (int j = 0; j < 16; j++) {
            if (c[j] != ref[j]) {
                asm volatile("add x29, x0, %0"
                             :
                             : "r"(c[j]));
                asm volatile("add x30, x0, %0"
                             :
                             : "r"(ref[j]));
                asm volatile("add x28, x0, %0"
                             :
                             : "r"(j+1)); // so that 0th index is 1st
                fail();
            }
        }
    }
    asm volatile("ecall");
}

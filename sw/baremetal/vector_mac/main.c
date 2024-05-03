#define LOOP_COUNT 1000u

void fail();
void pass();

unsigned char a[16] = {
    99, 228, 216, 184, 167, 203, 173, 107, 177, 205, 12, 128, 51, 95, 222, 212};
unsigned char b[16] = {
    152, 237, 93, 250, 200, 154, 185, 76, 1, 93, 58, 80, 88, 124, 125, 251};
unsigned int c[16] = {0};

const unsigned int ref[16] = {
    963072, 3458304, 1285632, 2944000, 2137600, 2000768, 2048320, 520448,
    11328, 1220160, 44544, 655360, 287232, 753920, 1776000, 3405568
};

int asm_add(unsigned int a, unsigned int b) {
    asm volatile("add %0, %1, %2" 
                 : "=r"(a)
                 : "r"(a), "r"(b));
    return a;
}

void set_c() {
    for (char i = 0; i < 16; i++) {
        c[i] = 0;
    }
}

void write_mismatch(unsigned int c, unsigned int ref, char j) {
    asm volatile("add x29, x0, %0"
                 :
                 : "r"(c));
    asm volatile("add x30, x0, %0"
                 :
                 : "r"(ref));
    asm volatile("add x28, x0, %0"
                 :
                 : "r"(j+1)); // so that 0th index is 1st
}

void main(void) {
    for (unsigned int i = 0; i < LOOP_COUNT; i++) {
        set_c();
        for (char j = 0; j < 64; j++) {
            for (char k = 0; k < 16; k++) {
                c[k] = asm_add(c[k], a[k] * b[k]);
            }
        }

        for (char j = 0; j < 16; j++) {
            if (c[j] != ref[j]) {
                write_mismatch(c[j], ref[j], j);
                fail();
            }
        }
    }
    pass();
}

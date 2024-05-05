#define LOOP_COUNT 1u

void fail();
void pass();

//#define EXPECTED_FIB_RESULT 55 // for n = 10
//#define EXPECTED_FIB_RESULT 610 // for n = 15
#define EXPECTED_FIB_RESULT 2584 // for n = 18
//#define EXPECTED_FIB_RESULT 6765 // for n = 20
//#define EXPECTED_FIB_RESULT 832040 // for n = 30
//#define EXPECTED_FIB_RESULT 102334155 // for n = 40

int fib(unsigned int n) {
    if (n <= 2)
        return 1;
    else
        return fib(n-1) + fib(n-2);
}

int main() {
    volatile unsigned int n = 18;
    
    for (unsigned int i = 0; i < LOOP_COUNT; i++) {
        unsigned int result = fib(n);
        
        asm volatile("add x29, x0, %0"
                    :
                    : "r"(EXPECTED_FIB_RESULT));
        asm volatile("add x30, x0, %0"
                    :
                    : "r"(result));
        if (result != EXPECTED_FIB_RESULT){
            asm volatile("addi x28, x0, 1"
                        :
                        : );
            fail();
        }
    }
        
    pass();
}

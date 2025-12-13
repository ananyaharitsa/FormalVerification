#include "CPU.h"

// Define assert locally to avoid <assert.h> issues if necessary
#ifdef __cplusplus
extern "C" {
#endif
void __assert_rtn(const char *, const char *, int, const char *) __attribute__((noreturn));
#ifdef __cplusplus
}
#endif

#define assert(e) ((e) ? (void)0 : __assert_rtn(__func__, __FILE__, __LINE__, #e))

extern int nondet_int();

void verify_program_logic() {
    CPU myCPU;
    
    // Symbolic Input
    int input_value = nondet_int();
    
    // Logic: 
    // lw x1, 0x4000 (We simulate this by assigning input_value)
    int x1 = input_value;
    
    // addi x3, x0, 42
    int x3 = 42;
    
    // beq x1, x3, loop (Infinite loop if equal)
    // Assertion: We want to prove we DO NOT loop.
    assert(x1 != x3);
}

int main() {
    verify_program_logic();
    return 0;
}
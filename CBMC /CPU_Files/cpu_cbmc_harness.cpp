#include "CPU.h" 
#include <assert.h> 

// CBMC Nondeterministic function declarations
extern unsigned char nondet_unsigned_char();
extern int nondet_int();
extern uint32_t nondet_uint32(); 

// --- CBMC HARNESS FUNCTION ---
void main_cbmc_harness() {
    // 1. Setup Environment 
    CPU myCPU; 
    unsigned char instMem[4096];
    int registers[32];
    
    // Nondeterministically fill instruction memory and registers
    for (int i = 0; i < 4096; i++) {
        instMem[i] = nondet_unsigned_char();
    }
    for (int i = 0; i < 32; i++) {
        registers[i] = nondet_int();
    }
    registers[0] = 0; // x0 must be 0

    myCPU.incPC(0); 
    
    // 2. FETCH and DECODE
    Instruction myInst(instMem, myCPU);
    Controller myController(myInst);
    ALU_Controller myALU_Control(myInst, myController.ALUOp);
    int32_t ImmValue = ImmGen(myInst);

    // Extract Register IDs 
    // instr is now uint32_t, so we use it directly
    int rs1 = (myInst.instr >> 15) & 0x1F; 
    int rs2 = (myInst.instr >> 20) & 0x1F; 
    int rd  = (myInst.instr >> 7)  & 0x1F; 

    int rs1Val = registers[rs1];
    int rs2Val_reg = registers[rs2];
    
    // 3. EXECUTE (ALU calculation)
    int rs2Val_mux = myController.AluSrc ? ImmValue : rs2Val_reg;
    int32_t ALU_Res = ALU_Result(rs1Val, rs2Val_mux, myALU_Control.ALUOp);


    // =======================================================
    // I. SAFETY PROPERTY: Data Memory Bounds Check
    // =======================================================
    if (myController.MemRe || myController.MemWr) {
        bool is_aligned = (ALU_Res % 4) == 0;
        bool in_bounds = (ALU_Res >= 0) && (ALU_Res < 4096 * 4); 
        
        assert(is_aligned && in_bounds);
    }


    // =======================================================
    // II. CONTROL LOGIC PROPERTIES
    // =======================================================
    uint32_t opcode = myInst.instr & 0x7F; 

    // A. R-Type Opcode Check (0x33)
    if (opcode == 0x33) {
        assert(myController.regWrite == 1); 
        assert(myController.AluSrc == 0); 
        assert(myController.MemRe == 0);
        assert(myController.MemWr == 0);
    }
    
    // B. Store Opcode Check (0x23)
    if (opcode == 0x23) { 
        assert(myController.MemWr == 1); 
        assert(myController.regWrite == 0); 
        assert(myController.AluSrc == 1); 
    }
    
    
    // =======================================================
    // III. FUNCTIONAL PROPERTIES 
    // =======================================================
    
    // A. ADDI Functional Check (Opcode 0x13, Funct3 0x0)
    if (opcode == 0x13 && ((myInst.instr >> 12) & 0x7) == 0x0) {
        // ADDI operation should always result in rs1Val + ImmValue
        assert(ALU_Res == (rs1Val + ImmValue)); 
        // ALUOp should be 2 (ADD)
        assert(myALU_Control.ALUOp == 2); 
    }

    // B. SRAI Functional Check (Opcode 0x13, Funct3 0x5)
    if (opcode == 0x13 && ((myInst.instr >> 12) & 0x7) == 0x5) {
        // Check Funct7 bit 30 (bit 5 of Funct7)
        uint32_t funct7_bit_5 = (myInst.instr >> 30) & 0b1; 
        
        if (funct7_bit_5 == 1) {
            // ALUOp should be 4 (SRAI)
            assert(myALU_Control.ALUOp == 4); 
            // Result should be shifted
            assert(ALU_Res == (rs1Val >> ImmValue));
        }
    }
}
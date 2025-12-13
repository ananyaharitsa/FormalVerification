#include "CPU.h"

CPU::CPU() {
    PC = 0;
    for (int i = 0; i < 4096; i++) dmemory[i] = 0;
    for (int i = 0; i < 32; i++) registers[i] = 0;
}

unsigned long CPU::readPC() { return PC; }
void CPU::incPC(unsigned long nextPC) { PC = nextPC; }

int32_t CPU::DataMemory(int MemWrite, int MemRead, int ALUResult, int rs2, bool word) {
    int index = ALUResult / 4;
    if (index < 0 || index >= 4096) return 0; 

    if (MemWrite) {
        if (word) dmemory[index] = rs2;
        else dmemory[index] = (dmemory[index] & 0xFFFFFF00) | (rs2 & 0xFF);
        return 0;
    } else if (MemRead) {
        if (word) return dmemory[index];
        else return dmemory[index] & 0xFF;
    }
    return 0;
}

CPU::StateSnapshot CPU::GetState() {
    StateSnapshot s;
    s.pc = PC;
    for(int i=0; i<32; i++) s.regs[i] = registers[i];
    for(int i=0; i<4096; i++) s.memory[i] = dmemory[i];
    return s;
}

void CPU::RestoreState(const StateSnapshot& s) {
    PC = s.pc;
    for(int i=0; i<32; i++) registers[i] = s.regs[i];
    for(int i=0; i<4096; i++) dmemory[i] = s.memory[i];
}

Instruction::Instruction(unsigned char instructionMem[], CPU cpu) {
    unsigned long currentPC = cpu.readPC();
    uint32_t combined = 0;
    for (int i = currentPC; i < currentPC + 4; ++i) {
        combined = (combined << 8) | instructionMem[i];
    }
    instr = toBigEndian(combined);
}

Controller::Controller(Instruction s) {
    uint32_t op = s.instr & 0x7F; // Raw bitmask
    opcode = op;
    regWrite = AluSrc = Branch = MemRe = MemWr = MemtoReg = 0;
    ALUOp = 0;

    if (op == 0x33) { regWrite=1; ALUOp=0b10; } // R-Type
    else if (op == 0x13) { regWrite=1; AluSrc=1; ALUOp=0b10; } // I-Type
    else if (op == 0x37) { regWrite=1; AluSrc=1; ALUOp=0b10; } // LUI
    else if (op == 0x03) { regWrite=1; AluSrc=1; MemRe=1; MemtoReg=1; ALUOp=0; } // Load
    else if (op == 0x23) { AluSrc=1; MemWr=1; ALUOp=0; } // Store
    else if (op == 0x6F) { regWrite=1; Branch=1; ALUOp=0b11; } // JAL
    else if (op == 0x63) { Branch=1; ALUOp=0b01; } // Branch
}

ALU_Controller::ALU_Controller(Instruction s, uint32_t ctrl) {
    uint32_t funct3 = (s.instr >> 12) & 0x7;
    uint32_t funct7 = (s.instr >> 25) & 0x7F;
    ALUOp = 0;
    
    if (ctrl == 0b10) { 
        if (funct3 == 0x0) ALUOp = 0b0010; // ADD
        else if (funct3 == 0x4) ALUOp = 0b0011; // XOR
        else if (funct3 == 0x6) ALUOp = 0b0001; // OR
        else if (funct3 == 0x5) ALUOp = 0b0100; // SRA
        if ((s.instr & 0x7F) == 0x37) ALUOp = 0b1000; // LUI case
    } 
    else if (ctrl == 0b00) ALUOp = 0b0010; // Load/Store -> ADD
    else if (ctrl == 0b01) ALUOp = 0b0110; // Branch -> SUB
    else if (ctrl == 0b11) ALUOp = 0b1111; // JAL -> Special
}

uint32_t toBigEndian(uint32_t value) {
    return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | 
           ((value & 0xFF0000) >> 8) | ((value >> 24) & 0xFF);
}

int32_t ImmGen(Instruction s) {
    uint32_t inst = s.instr;
    uint32_t op = inst & 0x7F;
    if (op == 0x33) return 0;
    if (op == 0x13 || op == 0x03) return (int32_t(inst) >> 20); 
    if (op == 0x23) { 
        return ((int32_t(inst) >> 25) << 5) | ((inst >> 7) & 0x1F);
    }
    if (op == 0x63) { 
        int32_t imm = ((inst >> 31) << 12) | ((inst & 0x7E000000) >> 20) |
                      ((inst & 0xF00) >> 8) | ((inst & 0x80) >> 7) << 1;
        return (imm << 19) >> 19; 
    }
    if (op == 0x6F) { 
        int32_t imm = ((inst >> 31) << 20) | (inst & 0xFF000) | ((inst & 0x100000) >> 9) | ((inst & 0x7FE00000) >> 20);
        return (imm << 11) >> 11; 
    }
    if (op == 0x37) return inst & 0xFFFFF000; 
    return 0;
}

int32_t ALU_Result(int x1, int x2, uint32_t op) {
    if (op == 2) return x1 + x2;
    if (op == 6) return x1 - x2;
    if (op == 0) return x1 & x2;
    if (op == 1) return x1 | x2;
    if (op == 3) return x1 ^ x2;
    if (op == 4) return x1 >> x2;
    if (op == 8) return x2; 
    return 0;
}
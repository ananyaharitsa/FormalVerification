#ifndef CPU_H
#define CPU_H

#include <stdint.h> // Only standard C integers allowed

class CPU {
public: 
    int dmemory[4096]; 
    unsigned long PC; 
    int registers[32]; 

    // Constants
    static const int MMIO_INPUT_ADDR = 0x4000; 
    static const int ISR_HANDLER_ADDR = 0x00000080;
    static const int EPC_REG = 30; 
    static const int MSTATUS_REG = 12;

    struct StateSnapshot {
        unsigned long pc;
        int regs[32];
        int memory[4096];

        bool operator==(const StateSnapshot& other) const {
            if (pc != other.pc) return false;
            for (int i = 0; i < 32; i++) if (regs[i] != other.regs[i]) return false;
            for (int i = 0; i < 4096; i++) if (memory[i] != other.memory[i]) return false;
            return true;
        }
    };

    CPU();
    unsigned long readPC();
    void incPC(unsigned long nextPC);
    int32_t DataMemory(int MemWrite, int MemRead, int ALUResult, int rs2, bool word);

    StateSnapshot GetState();
    void RestoreState(const StateSnapshot& s);
};

class Instruction { 
public:
    uint32_t instr; // Changed from bitset<32> to raw int
    Instruction(unsigned char instructionMem[], CPU cpu);
};

class Controller {
public:
    Controller(Instruction instr);
    bool regWrite, AluSrc, Branch, MemRe, MemWr, MemtoReg;
    uint32_t ALUOp; // Changed from bitset<2>
    uint32_t opcode; // Changed from bitset<7>
};

class ALU_Controller {
public:
    ALU_Controller(Instruction s, uint32_t ALUOp_FrmController);
    uint32_t ALUOp; // Changed from bitset<4>
};

uint32_t toBigEndian(uint32_t value);
int32_t ImmGen(Instruction s);
int32_t ALU_Result(int x1, int x2, uint32_t ALUOp);

#endif
#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm> // For std::copy

using namespace std;

class CPU {
public: // Changed to public so ModelChecker can access state directly
    int dmemory[4096]; 
    unsigned long PC; 
    int registers[32]; 

    // Snapshot structure to hold a specific CPU state
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

    // Helper functions for the Model Checker
    StateSnapshot GetState() {
        StateSnapshot s;
        s.pc = PC;
        std::copy(std::begin(registers), std::end(registers), std::begin(s.regs));
        std::copy(std::begin(dmemory), std::end(dmemory), std::begin(s.memory));
        return s;
    }

    void RestoreState(const StateSnapshot& s) {
        PC = s.pc;
        std::copy(std::begin(s.regs), std::end(s.regs), std::begin(registers));
        std::copy(std::begin(s.memory), std::end(s.memory), std::begin(dmemory));
    }
};

class Instruction { 
public:
    bitset<32> instr = 0;
    Instruction(unsigned char instructionMem[], CPU cpu);
};

class Controller {
public:
    Controller(Instruction instr);
    bool regWrite = 0, AluSrc = 0, Branch = 0, MemRe = 0, MemWr = 0, MemtoReg = 0;
    bitset<2> ALUOp;
    bitset<7> opcode;
};

class ALU_Controller {
public:
    ALU_Controller(Instruction s, bitset<2> ALUOp_FrmController);
    bitset<4> ALUOp;
};

#pragma once
uint32_t toBigEndian(uint32_t value);
int32_t ImmGen(Instruction s);
int32_t ALU_Result(int x1, int x2, bitset<4> ALUOp);
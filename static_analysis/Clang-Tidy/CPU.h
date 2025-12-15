#pragma once

#include <cstdint>
#include <bitset>

// Forward declaration so header doesn't include CPU.cpp
class CPU;

class Instruction {
public:
    // cppcheck-suppress unusedStructMember
    std::bitset<32> instr{0};
    Instruction(const unsigned char instructionMem[], const CPU &cpu);
};

class Controller {
public:
    explicit Controller(const Instruction &instr);
    bool regWrite = false;
    bool AluSrc = false;
    bool Branch = false;
    bool MemRe = false;
    bool MemWr = false;
    bool MemtoReg = false;
    // cppcheck-suppress unusedStructMember
    std::bitset<2> ALUOp;
    // cppcheck-suppress unusedStructMember
    std::bitset<7> opcode;
};

class ALU_Controller {
public:
    ALU_Controller(const Instruction &s, std::bitset<2> ALUOp_FrmController);
    // cppcheck-suppress unusedStructMember
    std::bitset<4> ALUOp;
};

class CPU {
private:
    // cppcheck-suppress unusedStructMember
    int32_t dmemory[4096] = {0}; // data memory (words)
    unsigned long PC = 0;

public:
    CPU();
    unsigned long readPC() const;
    void incPC(unsigned long nextPC);
    int32_t DataMemory(int MemWrite, int MemRead, int ALUResult, int rs2, bool word);
};

// helper function signatures
uint32_t toBigEndian(uint32_t value);
int32_t ImmGen(const Instruction &s);
int32_t ALU_Result(int x1, int x2, std::bitset<4> ALUOp);
./cpusim Test/trace/24instMem-r.txt

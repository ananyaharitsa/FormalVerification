#include "CPU.h"

#include <bitset>
#include <cstdint>
#include <iostream>

// Constructor
CPU::CPU()
    : dmemory{}, PC(0)
{
}

// Instruction constructor
Instruction::Instruction(const unsigned char instructionMem[], const CPU &cpu)
{
    unsigned long currentPC = cpu.readPC();
    uint32_t combined = 0u;

    for (unsigned long idx = currentPC; idx < currentPC + 4 && idx < 4096; ++idx) {
        uint32_t byte = static_cast<uint8_t>(instructionMem[idx]);
        combined = (combined << 8) | byte;
    }

    instr = std::bitset<32>(toBigEndian(combined));
}

// Controller
Controller::Controller(const Instruction &instr)
{
    std::bitset<32> instrLow = instr.instr & std::bitset<32>(0x7F);

    if (instrLow == std::bitset<32>(0b0110011)) { // R-type
        regWrite = true; AluSrc = false; Branch = false;
        MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0b10);
        opcode = std::bitset<7>(0b0110011);
    }
    else if (instrLow == std::bitset<32>(0b0010011) ||
             instrLow == std::bitset<32>(0b0110111)) { // I-type or U-type
        regWrite = true; AluSrc = true; Branch = false;
        MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0b10);
        opcode = std::bitset<7>(instrLow.to_ulong());
    }
    else if (instrLow == std::bitset<32>(0b0000011)) { // LOAD
        regWrite = true; AluSrc = true; Branch = false;
        MemRe = true; MemWr = false; MemtoReg = true;
        ALUOp = std::bitset<2>(0b00);
        opcode = std::bitset<7>(0b0000011);
    }
    else if (instrLow == std::bitset<32>(0b0100011)) { // STORE
        regWrite = false; AluSrc = true; Branch = false;
        MemRe = false; MemWr = true; MemtoReg = false;
        ALUOp = std::bitset<2>(0b00);
        opcode = std::bitset<7>(0b0100011);
    }
    else if (instrLow == std::bitset<32>(0b1101111)) { // JAL
        regWrite = true; AluSrc = false; Branch = true;
        MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0b11);
        opcode = std::bitset<7>(0b1101111);
    }
    else if (instrLow == std::bitset<32>(0b1100011)) { // BEQ
        regWrite = false; AluSrc = false; Branch = true;
        MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0b01);
        opcode = std::bitset<7>(0b1100011);
    }
    else {
        regWrite = false; AluSrc = false; Branch = false;
        MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0);
        opcode = std::bitset<7>(0);
    }
}

// ALU Controller
ALU_Controller::ALU_Controller(const Instruction &s, std::bitset<2> ALUOp_FrmController)
{
    if (ALUOp_FrmController == std::bitset<2>(0b10)) {
        std::bitset<32> maskFunct3(0x7000);

        if ((maskFunct3 & s.instr) == std::bitset<32>(0)) {
            ALUOp = std::bitset<4>(0b0010);
        }
        else if ((maskFunct3 & s.instr) == std::bitset<32>(0x4000)) {
            ALUOp = std::bitset<4>(0b0011);
        }
        else if ((maskFunct3 & s.instr) == std::bitset<32>(0x6000)) {
            ALUOp = std::bitset<4>(0b0001);
        }
        else if ((maskFunct3 & s.instr) == std::bitset<32>(0x5000)) {
            ALUOp = std::bitset<4>(0b0100);
        }
        else {
            ALUOp = std::bitset<4>(0b0010);
        }
    }
    else if (ALUOp_FrmController == std::bitset<2>(0b00)) {
        ALUOp = std::bitset<4>(0b0010);
    }
    else if (ALUOp_FrmController == std::bitset<2>(0b01)) {
        ALUOp = std::bitset<4>(0b0110);
    }
    else if (ALUOp_FrmController == std::bitset<2>(0b11)) {
        ALUOp = std::bitset<4>(0b1111);
    }
    else {
        ALUOp = std::bitset<4>(0);
    }
}

// CPU functions
unsigned long CPU::readPC() const
{
    return PC;
}

void CPU::incPC(unsigned long nextPC)
{
    PC = nextPC;
}

// Data memory
int32_t CPU::DataMemory(int MemWrite, int MemRead, int ALUResult, int rs2, bool word)
{
    const int DMEM_SIZE = 4096;

    if (ALUResult < 0 || ALUResult >= (DMEM_SIZE * 4)) {
        return 0;
    }

    int index = ALUResult / 4;

    if ((MemWrite != 0) && word) {
        dmemory[index] = rs2;
        return 0;
    }

    if ((MemWrite != 0) && !word) {
        dmemory[index] = rs2 & 0xFF;
        return 0;
    }

    if ((MemRead != 0) && word) {
        return dmemory[index];
    }

    if ((MemRead != 0) && !word) {
        return (dmemory[index] & 0xFF);
    }

    return 0;
}

// Endianness conversion
uint32_t toBigEndian(uint32_t value)
{
    uint32_t byte0 = (value & 0x000000FF) << 24;
    uint32_t byte1 = (value & 0x0000FF00) << 8;
    uint32_t byte2 = (value & 0x00FF0000) >> 8;
    uint32_t byte3 = (value & 0xFF000000) >> 24;

    return byte0 | byte1 | byte2 | byte3;
}

// Immediate generator
int32_t ImmGen(const Instruction &s)
{
    using std::bitset;
    bitset<32> instr = s.instr;
    uint32_t opcode = (instr & bitset<32>(0x7F)).to_ulong();

    if (opcode == 0b0110011) return 0;

    if (opcode == 0b0010011) {
        bitset<32> maskFunct3(0x7000);
        uint32_t funct3 = (maskFunct3 & instr).to_ulong();
        if (funct3 == 0x6000) {
            uint32_t imm = (instr >> 20).to_ulong() & 0xFFFu;
            return (imm & 0x800)
                ? (static_cast<int32_t>(imm) | 0xFFFFF000)
                : static_cast<int32_t>(imm);
        }
        else if (funct3 == 0x5000) {
            uint32_t imm = (instr >> 20).to_ulong() & 0x1Fu;
            return static_cast<int32_t>(imm);
        }
    }

    if (opcode == 0b0110111) {
        return static_cast<int32_t>((instr & bitset<32>(0xFFFFF000)).to_ulong());
    }

    if (opcode == 0b0000011) {
        uint32_t imm = (instr >> 20).to_ulong() & 0xFFFu;
        return (imm & 0x800)
            ? (static_cast<int32_t>(imm) | 0xFFFFF000)
            : static_cast<int32_t>(imm);
    }

    if (opcode == 0b0100011) {
        uint32_t imm_hi = ((instr >> 25) & bitset<32>(0x7F)).to_ulong();
        uint32_t imm_lo = ((instr >> 7) & bitset<32>(0x1F)).to_ulong();
        uint32_t imm = (imm_hi << 5) | imm_lo;
        return (imm & 0x800)
            ? (static_cast<int32_t>(imm) | 0xFFFFF000)
            : static_cast<int32_t>(imm);
    }

    if (opcode == 0b1100011) {
        int32_t imm = 0;
        imm |= ((instr[31] ? 1 : 0) << 12);
        imm |= ((instr[7] ? 1 : 0) << 11);
        imm |= (((instr >> 25) & bitset<32>(0x3F)).to_ulong() << 5);
        imm |= (((instr >> 8) & bitset<32>(0xF)).to_ulong() << 1);
        if (imm & (1 << 12)) imm |= 0xFFFFE000;
        return imm;
    }

    if (opcode == 0b1101111) {
        int32_t imm = 0;
        imm |= ((instr[31] ? 1 : 0) << 20);
        imm |= (((instr >> 21) & bitset<32>(0x3FF)).to_ulong() << 1);
        imm |= (((instr >> 20) & bitset<32>(0x1)).to_ulong() << 11);
        imm |= (((instr >> 12) & bitset<32>(0xFF)).to_ulong() << 12);
        if (imm & (1 << 20)) imm |= 0xFFF00000;
        return imm;
    }

    return 0;
}

// ALU
int32_t ALU_Result(int x1, int x2, std::bitset<4> ALUOp)
{
    if (ALUOp == std::bitset<4>(0b0010)) return x1 + x2;
    if (ALUOp == std::bitset<4>(0b0110)) return x1 - x2;
    if (ALUOp == std::bitset<4>(0b0000)) return x1 & x2;
    if (ALUOp == std::bitset<4>(0b0001)) return x1 | x2;
    if (ALUOp == std::bitset<4>(0b0011)) return x1 ^ x2;
    if (ALUOp == std::bitset<4>(0b0100)) return x1 >> x2;
    if (ALUOp == std::bitset<4>(0b1000)) return x2;
    if (ALUOp == std::bitset<4>(0b1111)) return 0;

    return 0;
}

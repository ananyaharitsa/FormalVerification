#include "CPU.h"

#include <bitset>
#include <cstdint>
#include <iostream>

// Constructor
CPU::CPU()
    : dmemory{}, PC(0)
{
    // dmemory zero-initialized in the declaration
}

// Instruction constructor: read up to 4 bytes from instruction memory at cpu.PC
Instruction::Instruction(const unsigned char instructionMem[], const CPU &cpu)
{
    unsigned long currentPC = cpu.readPC();
    //Load the First 
    // Combine the 8 ASCII hex characters into a 32-bit integer
    uint32_t combined = 0u;
    // read up to 4 bytes starting at currentPC (safety: do not read past 4096)
    for (unsigned long idx = currentPC; idx < currentPC + 4 && idx < 4096; ++idx) {
        uint32_t byte = static_cast<uint8_t>(instructionMem[idx]);
        combined = (combined << 8) | byte;
    }
    instr = std::bitset<32>(toBigEndian(combined));
}

// Controller definition (parameter name matches header)
Controller::Controller(const Instruction &instr)
{
    //Based on the opcode, find the type:
    //R-Type
    const std::bitset<32> opcode_R(0b0110011);
    //I -Type
    const std::bitset<32> opcode_I(0b0010011);
    //U-Type(LUI)
    const std::bitset<32> opcode_U(0b0110111);
    //LOAD WORD/BYTE
    const std::bitset<32> opcode_L(0b0000011);
    //STORE WORD/BYTE
    const std::bitset<32> opcode_S(0b0100011);
    //J-Type(JAL)
    const std::bitset<32> opcode_J(0b1101111);
    //B-Type (BEQ)
    const std::bitset<32> opcode_B(0b1100011);

    std::bitset<32> bits = instr.instr;
    std::bitset<32> instrLow = bits & std::bitset<32>(0x7F);

    if (instrLow == opcode_R) {
        regWrite = true; AluSrc = false; Branch = false; MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0b10); opcode = std::bitset<7>(0b0110011);
    }
    else if (instrLow == opcode_I) {
        regWrite = true; AluSrc = true; Branch = false; MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0b10); opcode = std::bitset<7>(0b0010011);
    }
    else if (instrLow == opcode_U) {
        regWrite = true; AluSrc = true; Branch = false; MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0b10); opcode = std::bitset<7>(0b0110111);
    }
    else if (instrLow == opcode_L) {
        regWrite = true; AluSrc = true; Branch = false; MemRe = true; MemWr = false; MemtoReg = true;
        ALUOp = std::bitset<2>(0b00); opcode = std::bitset<7>(0b0000011);
    }
    else if (instrLow == opcode_S) {
        regWrite = false; AluSrc = true; Branch = false; MemRe = false; MemWr = true; MemtoReg = false;
        ALUOp = std::bitset<2>(0b00); opcode = std::bitset<7>(0b0100011);
    }
    else if (instrLow == opcode_J) {
        regWrite = true; AluSrc = false; Branch = true; MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0b11); opcode = std::bitset<7>(0b1101111);
    }
    else if (instrLow == opcode_B) {
        regWrite = false; AluSrc = false; Branch = true; MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0b01); opcode = std::bitset<7>(0b1100011);
    }
    else {
        // safe defaults
        regWrite = false; AluSrc = false; Branch = false; MemRe = false; MemWr = false; MemtoReg = false;
        ALUOp = std::bitset<2>(0b00); opcode = std::bitset<7>(0);
    }
}

ALU_Controller::ALU_Controller(const Instruction &s, std::bitset<2> ALUOp_FrmController)
{
    if (ALUOp_FrmController == std::bitset<2>(0b10)) {
        std::bitset<32> maskFunct3(0x7000);
        if ((maskFunct3 & s.instr) == std::bitset<32>(0)) {
            ALUOp = std::bitset<4>(0b0010); // ADD
        }
        else if ((maskFunct3 & s.instr) == std::bitset<32>(0x4000)) {
            ALUOp = std::bitset<4>(0b0011); // XOR
        }
        else if ((maskFunct3 & s.instr) == std::bitset<32>(0x6000)) {
            ALUOp = std::bitset<4>(0b0001); // ORI
        }
        else if ((maskFunct3 & s.instr) == std::bitset<32>(0x5000)) {
            ALUOp = std::bitset<4>(0b0100); // SRAI
        }
        else {
            if (((s.instr & std::bitset<32>(0x7F)) == std::bitset<32>(0b0110111))) {
                ALUOp = std::bitset<4>(0b1000); // LUI
            } else {
                ALUOp = std::bitset<4>(0b0010); // default ADD
            }
        }
    }
    else if (ALUOp_FrmController == std::bitset<2>(0b00)) {
        ALUOp = std::bitset<4>(0b0010); // address calc (ADD)
    }
    else if (ALUOp_FrmController == std::bitset<2>(0b01)) {
        ALUOp = std::bitset<4>(0b0110); // BEQ -> SUB
    }
    else if (ALUOp_FrmController == std::bitset<2>(0b11)) {
        ALUOp = std::bitset<4>(0b1111); // JAL
    }
    else {
        ALUOp = std::bitset<4>(0);
    }
}

// CPU member functions
unsigned long CPU::readPC() const
{
    return PC;
}
void CPU::incPC(unsigned long nextPC)
{
    //Increasing by 8 (Hexadecimal Bits (4 Bytes))
    PC = nextPC;
}

int32_t CPU::DataMemory(int MemWrite, int MemRead, int ALUResult, int rs2, bool word)
{
    const int DMEM_SIZE = 4096;
    // ALUResult is a byte address: check bounds (prevent OOB)
    if (ALUResult < 0 || ALUResult >= (DMEM_SIZE * 4)) {
        std::cerr << "DataMemory: out-of-bounds access: " << ALUResult << std::endl;
        return 0;
    }
    int index = ALUResult / 4;
    if (index < 0 || index >= DMEM_SIZE) {
        std::cerr << "DataMemory: bad index: " << index << std::endl;
        return 0;
    }

    if (MemWrite && word) {
        dmemory[index] = rs2;
        return 0;
    }
    else if (MemWrite && !word) {
        dmemory[index] = rs2 & 0xFF;
        return 0;
    }
    else if (MemRead && word) {
        return dmemory[index];
    }
    else if (MemRead && !word) {
        return (dmemory[index] & 0xFF);
    }
    return 0;
}
//////////////////////////////////////////////////////////////////////




//OTHER FUNCTIONS

// LITTLE ENDIAN INSTRUCTION TO BIG ENDIAN
uint32_t toBigEndian(uint32_t value) {
    // Extract and shift each byte to its new position
    uint32_t byte0 = (value & 0x000000FF) << 24;
    uint32_t byte1 = (value & 0x0000FF00) << 8;
    uint32_t byte2 = (value & 0x00FF0000) >> 8;
    uint32_t byte3 = (value & 0xFF000000) >> 24;

    // Combine the bytes to get the big-endian result
    return byte0 | byte1 | byte2 | byte3;
}

int32_t ImmGen(const Instruction &s)
{
    using std::bitset;
    bitset<32> instr = s.instr;
    uint32_t opcode = (instr & bitset<32>(0x7F)).to_ulong();

    if (opcode == 0b0110011) return 0; // R-type

    if (opcode == 0b0010011) { // I-type
        bitset<32> maskFunct3(0x7000);
        uint32_t funct3 = (maskFunct3 & instr).to_ulong();
        if (funct3 == 0x6000) {
            uint32_t imm = (instr >> 20).to_ulong() & 0xFFFu;
            int32_t v = (imm & 0x800) ? (static_cast<int32_t>(imm) | 0xFFFFF000) : static_cast<int32_t>(imm);
            return v;
        } else if (funct3 == 0x5000) {
            uint32_t imm = (instr >> 20).to_ulong() & 0x1Fu;
            return static_cast<int32_t>(imm);
        }
    }

    if (opcode == 0b0110111) { // LUI
        uint32_t top20 = (instr & bitset<32>(0xFFFFF000)).to_ulong();
        return static_cast<int32_t>(top20);
    }

    if (opcode == 0b0000011) {
        uint32_t imm = (instr >> 20).to_ulong() & 0xFFFu;
        int32_t v = (imm & 0x800) ? (static_cast<int32_t>(imm) | 0xFFFFF000) : static_cast<int32_t>(imm);
        return v;
    }

    if (opcode == 0b0100011) { // S-type
        uint32_t imm_hi = ((instr >> 25) & bitset<32>(0x7F)).to_ulong();
        uint32_t imm_lo = ((instr >> 7) & bitset<32>(0x1F)).to_ulong();
        uint32_t imm = (imm_hi << 5) | imm_lo;
        int32_t v = (imm & 0x800) ? (static_cast<int32_t>(imm) | 0xFFFFF000) : static_cast<int32_t>(imm);
        return v;
    }

    if (opcode == 0b1100011) { // B-type
        int32_t imm = 0;
        imm |= ((instr[31] ? 1:0) << 12);
        imm |= ((instr[7] ? 1:0) << 11);
        imm |= (((instr >> 25) & bitset<32>(0x3F)).to_ulong() << 5);
        imm |= (((instr >> 8) & bitset<32>(0xF)).to_ulong() << 1);
        if (imm & (1 << 12)) imm |= 0xFFFFE000;
       return imm;
    }

    if (opcode == 0b1101111) { // J-type
        int32_t imm = 0;
        imm |= ((instr[31] ? 1:0) << 20);
        imm |= (((instr >> 21) & bitset<32>(0x3FF)).to_ulong() << 1);
        imm |= (((instr >> 20) & bitset<32>(0x1)).to_ulong() << 11);
        imm |= (((instr >> 12) & bitset<32>(0xFF)).to_ulong() << 12);
        if (imm & (1 << 20)) imm |= 0xFFF00000;
        return imm;
    }

    return 0;
}

int32_t ALU_Result(int x1, int x2, std::bitset<4> ALUOp)
{
    //ADD
    if (ALUOp == std::bitset<4>(0b0010)) {
        return x1 + x2;
    }
    //SUBTRACT//BEQ
    else if (ALUOp == std::bitset<4>(0b0110)) {
        return x1 - x2;
    }
    //AND
    else if (ALUOp == std::bitset<4>(0b0000)) {
        return x1 & x2;
    }
    //ORI
    else if (ALUOp == std::bitset<4>(0b0001)) {
        return x1 | x2;
    }
    //XOR
    else if (ALUOp == std::bitset<4>(0b0011)) {
        return x1 ^ x2;
    }
    //SRAI
    else if (ALUOp == std::bitset<4>(0b0100)) {
        return x1 >> x2;
    }
    //LUI
    else if (ALUOp == std::bitset<4>(0b1000)) {
        return x2;
    }
    //JAL
    else if (ALUOp == std::bitset<4>(0b1111)) {
    //RETURN 0
            return 0;
    }

    // explicit fallback (distinct behaviour from other branches)
    std::cerr << "ALU_Result: unexpected ALUOp = " << ALUOp.to_ulong() << " -> defaulting to 0\n";
    return 0;
}

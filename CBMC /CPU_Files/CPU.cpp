#include "CPU.h" 
#include <assert.h> 

//////////////////////////////////////////////////////////////////////
//CONSTRUCTORS
//////////////////////////////////////////////////////////////////////

CPU::CPU()
{
    PC = 0; //set PC to 0
    for (int i = 0; i < 4096; i++) //copy instrMEM
    {
        dmemory[i] = (0);
    }
}

// Instruction Fetch 
Instruction::Instruction(unsigned char instructionMem[], CPU cpu)
{
    unsigned long currentPC = cpu.readPC();
    // Combine the 8 ASCII hex characters into a 32-bit integer
    uint32_t combined = 0;
    for (int i = currentPC; i < currentPC + 4; ++i) {
        unsigned char ch = instructionMem[i];
        uint32_t hexValue = static_cast<uint32_t>(ch);
        
        // Shift combined left by 8 bits (1 BYTE) to make space 
        combined = (combined << 8) | hexValue;
    }

    // Update instr (uint32_t) from the 32-bit integer
    instr = toBigEndian(combined);
}

Controller::Controller(Instruction s)
{
    // Based on the opcode, find the type:
    // Constants defined as integer literals (binary or hex)
    
    //R-Type (0110011) -> 0x33
    uint32_t opcode_R = 0x33;
    //I -Type (0010011) -> 0x13
    uint32_t opcode_I = 0x13;
    //U-Type (LUI) (0110111) -> 0x37
    uint32_t opcode_U = 0x37;
    //LOAD WORD/BYTE (0000011) -> 0x03
    uint32_t opcode_L = 0x03;
    //STORE WORD/BYTE (0100011) -> 0x23
    uint32_t opcode_S = 0x23;
    //J-Type (JAL) (1101111) -> 0x6F
    uint32_t opcode_J = 0x6F;
    //B-Type (BEQ) (1100011) -> 0x63
    uint32_t opcode_B = 0x63;

    uint32_t currentOpcode = s.instr & 0x7F; // Extract lowest 7 bits

    //R-TYPE
    if (currentOpcode == opcode_R) {
        regWrite = 1;
        AluSrc = 0;
        Branch = 0;
        MemRe = 0;
        MemWr = 0;
        MemtoReg = 0;
        ALUOp = 2; // 0b10
        opcode = 0x33;
    }
    //I-TYPE
    else if (currentOpcode == opcode_I) {
        regWrite = 1;
        AluSrc = 1;
        Branch = 0;
        MemRe = 0;
        MemWr = 0;
        MemtoReg = 0;
        ALUOp = 2; // 0b10
        opcode = 0x13;
    }
    //U-TYPE
    else if (currentOpcode == opcode_U) {
        regWrite = 1;
        AluSrc = 1;
        Branch = 0;
        MemRe = 0;
        MemWr = 0;
        MemtoReg = 0;
        ALUOp = 2; // 0b10
        opcode = 0x37;
    }
    //LOAD
    else if (currentOpcode == opcode_L) {
        regWrite = 1;
        AluSrc = 1;
        Branch = 0;
        MemRe = 1;
        MemWr = 0;
        MemtoReg = 1;
        ALUOp = 0; // 0b00
        opcode = 0x03;
    }
    //STORE
    else if (currentOpcode == opcode_S) {
        regWrite = 0;
        AluSrc = 1;
        Branch = 0;
        MemRe = 0;
        MemWr = 1;
        MemtoReg = 0;
        ALUOp = 0; // 0b00
        opcode = 0x23;
    }
    else if (currentOpcode == opcode_J) {
        regWrite = 1;
        AluSrc = 0;
        Branch = 1;
        MemRe = 0;
        MemWr = 0;
        MemtoReg = 0;
        ALUOp = 3; // 0b11
        opcode = 0x6F;
    }
    else if (currentOpcode == opcode_B) {
        regWrite = 0;
        AluSrc = 0;
        Branch = 1;
        MemRe = 0;
        MemWr = 0;
        MemtoReg = 0;
        ALUOp = 1; // 0b01
        opcode = 0x63;
    }
}


ALU_Controller::ALU_Controller(Instruction s, uint8_t ALUOp_FrmController)
{
    // If it is R-Type/I-type/U-Type
    if (ALUOp_FrmController == 2) { // 0b10
        //Checking for FUNCT3 since only have to account for ADD and XOR (R-TYPE)
        uint32_t maskFunct3 = 0x7000; // Just 1's in the Funct3 Section (bits 12-14)

        //If its ADD (Funct3 is 0)
        if ((maskFunct3 & s.instr) == 0) {
            ALUOp = 2; // 0b0010 (ADD)
        }
        //If its XOR (Funct3 is 0x4000)
        else if ((maskFunct3 & s.instr) == 0x4000) {
            ALUOp = 3; // 0b0011 (XOR)
        }

        //Checking for FUNCT3 since only have to account for ORI and SRAI (I-TYPE)
        //If its ORI (Funct3 is 0x6000)
        if ((maskFunct3 & s.instr) == 0x6000) {
            ALUOp = 1; // 0b0001 (OR)
        }
        //If its SRAI (Funct3 is 0x5000)
        else if ((maskFunct3 & s.instr) == 0x5000) {
            ALUOp = 4; // 0b0100 (SRAI/Shift)
        }

        //CHECKING only OPCODE for LUI
        //U-Type(LUI) -> 0x37
        uint32_t opcode_U = 0x37;
        if ((s.instr & 0x7F) == opcode_U) {
            ALUOp = 8; // 0b1000 (LUI)
        }
    }
    //If its LOAD/STORE
    else if (ALUOp_FrmController == 0) {
        ALUOp = 2; // 0b0010 (ADD for address calc)
    }
    //If its BRANCH (BEQ)
    else if (ALUOp_FrmController == 1) {
        ALUOp = 6; // 0b0110 (SUB for comparison)
    }
    //If its JUMP (JAL)
    else if (ALUOp_FrmController == 3) {
        ALUOp = 15; // 0b1111 (JAL special op)
    }
}

//////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////

unsigned long CPU::readPC()
{
    return PC;
}
void CPU::incPC(unsigned long nextPC)
{
    PC = nextPC;
}

int32_t CPU::DataMemory(int MemWrite, int MemRead, int ALUResult, int rs2, bool word)
{
    // Memory safety is checked by CBMC harness
    
    // Memory is 4096 words
    if (MemWrite && word) {
        dmemory[ALUResult / 4] = rs2;
        return 0;
    }
    //IF the store is just a byte
    else if (MemWrite && !word) {
        dmemory[ALUResult / 4] = rs2 & 0xFF;
        return 0;
    }
    else if (MemRead && word) {
        return dmemory[ALUResult / 4];
    }
    //IF the load is just a byte
    else if (MemRead && !word) {
        return (dmemory[ALUResult / 4] & 0xFF);
    }
    else
        return -1;
}

// LITTLE ENDIAN INSTRUCTION TO BIG ENDIAN
uint32_t toBigEndian(uint32_t value) {
    uint32_t byte0 = (value & 0x000000FF) << 24;
    uint32_t byte1 = (value & 0x0000FF00) << 8;
    uint32_t byte2 = (value & 0x00FF0000) >> 8;
    uint32_t byte3 = (value & 0xFF000000) >> 24;
    return byte0 | byte1 | byte2 | byte3;
}


int32_t ImmGen(Instruction s) {
    uint32_t opcode = s.instr & 0x7F;

    //R-TYPE
    if (opcode == 0x33) { // 0110011
        return 0;
    }
    //I-TYPE
    else if (opcode == 0x13) { // 0010011
        uint32_t maskFunct3 = 0x7000;

        //If its ORI
        if ((maskFunct3 & s.instr) == 0x6000) {
            uint32_t top12Bits = (s.instr >> 20) & 0xFFF; // Extract 12 bits

            int32_t signedValue;
            // Check sign bit (bit 11 of the 12 bits)
            if ((top12Bits >> 11) & 1) { 
                signedValue = static_cast<int32_t>(top12Bits | 0xFFFFF000); // Sign extend
            }
            else {
                signedValue = static_cast<int32_t>(top12Bits);
            }
            return signedValue;

        }
        //If its SRAI
        else if ((maskFunct3 & s.instr) == 0x5000) {
            uint32_t shiftbits = (s.instr >> 20) & 0x1F; // Extract 5 bits

            // Shift amount is always positive/unsigned effectively for shift ops
            return static_cast<int32_t>(shiftbits);
        }
    }
    //U-TYPE
    else if (opcode == 0x37) { // 0110111
        uint32_t top20Bits = (s.instr & 0xFFFFF000);
        return static_cast<int32_t>(top20Bits);
    } 
    //LOAD
    else if (opcode == 0x03) { // 0000011
        uint32_t top12Bits = (s.instr >> 20) & 0xFFF;

        int32_t signedValue;
        if ((top12Bits >> 11) & 1) {
            signedValue = static_cast<int32_t>(top12Bits | 0xFFFFF000);
        }
        else {
            signedValue = static_cast<int32_t>(top12Bits);
        }
        return signedValue;
    }
    //STORE
    else if (opcode == 0x23) { // 0100011
        //Combining bits [31:25] and [11:7]
        uint32_t imm11_5 = (s.instr >> 25) & 0x7F;
        uint32_t imm4_0  = (s.instr >> 7) & 0x1F;
        
        uint32_t top12Bits = (imm11_5 << 5) | imm4_0;

        int32_t signedValue;
        if ((top12Bits >> 11) & 1) { 
            signedValue = static_cast<int32_t>(top12Bits | 0xFFFFF000);
        }
        else {
            signedValue = static_cast<int32_t>(top12Bits);
        }
        return signedValue;

    }
    //BRANCH
    else if (opcode == 0x63) { // 1100011
       //Combining bits: [31], [7], [30:25], [11:8]
       // Bit 12: instr[31]
       // Bit 11: instr[7]
       // Bits 10:5: instr[30:25]
       // Bits 4:1: instr[11:8]
       // Bit 0: 0
       
       uint32_t bit12 = (s.instr >> 31) & 1;
       uint32_t bit11 = (s.instr >> 7) & 1;
       uint32_t bits10_5 = (s.instr >> 25) & 0x3F;
       uint32_t bits4_1 = (s.instr >> 8) & 0xF;

       uint32_t top13Bits = (bit12 << 12) | (bit11 << 11) | (bits10_5 << 5) | (bits4_1 << 1);

        int32_t signedValue;
        if ((top13Bits >> 12) & 1) { 
            signedValue = static_cast<int32_t>(top13Bits | 0xFFFFE000); // Sign extend 13 bits
        }
        else {
            signedValue = static_cast<int32_t>(top13Bits);
        }
        return signedValue;

    }    
    //JUMP
    else if (opcode == 0x6F) { // 1101111
       //Bits: [31], [19:12], [20], [30:21]
       
       uint32_t bit20 = (s.instr >> 31) & 1;
       uint32_t bits19_12 = (s.instr >> 12) & 0xFF;
       uint32_t bit11 = (s.instr >> 20) & 1;
       uint32_t bits10_1 = (s.instr >> 21) & 0x3FF;

       uint32_t top21Bits = (bit20 << 20) | (bits19_12 << 12) | (bit11 << 11) | (bits10_1 << 1);

        int32_t signedValue;
        if ((top21Bits >> 20) & 1) { 
            signedValue = static_cast<int32_t>(top21Bits | 0xFFE00000); // Sign extend 21 bits
        }
        else {
            signedValue = static_cast<int32_t>(top21Bits);
        }
        return signedValue;

    }
    else {
        return 0;
    }
    return 0;    
}

int32_t ALU_Result(int x1, int x2, uint8_t ALUOp)
{
    //ADD
    if (ALUOp == 2) { // 0b0010
        return x1 + x2;
    }
    //SUBTRACT//BEQ
    else if (ALUOp == 6) { // 0b0110
        return x1 - x2;
    }
    //AND
    else if (ALUOp == 0) { // 0b0000
        return x1 & x2;
    }
    //ORI
    else if (ALUOp == 1) { // 0b0001
        return x1 | x2;
    }
    //XOR
    else if (ALUOp == 3) { // 0b0011
        return x1 ^ x2;
    }
    //SRAI
    else if (ALUOp == 4) { // 0b0100
        return x1 >> x2;
    }
    //LUI
    else if (ALUOp == 8) { // 0b1000
        return x2;
    }
    //JAL
    else if (ALUOp == 15) { // 0b1111
        return 0;
    }
    else {
        return 0;
    }
}
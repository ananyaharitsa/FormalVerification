#include "CPU.h"



//CONSTRUCTORS

CPU::CPU()
{
	PC = 0; //set PC to 0
	for (int i = 0; i < 4096; i++) //copy instrMEM
	{
		dmemory[i] = (0);
	}
}

unsigned long CPU::readPC()
{
    return PC;
}
void CPU::incPC(unsigned long nextPC)
{
    //Increasing by 8 (Hexadecimal Bits (4 Bytes))
    PC = nextPC;
}

//Insturction Fetch
Instruction::Instruction(unsigned char instructionMem[], CPU cpu)
{
    unsigned long currentPC = cpu.readPC();
    //Load the First 
    // Combine the 8 ASCII hex characters into a 32-bit integer
    uint32_t combined = 0;
    for (int i = currentPC; i < currentPC + 4; ++i) {
        unsigned char ch = instructionMem[i];
        uint32_t hexValue = static_cast<uint32_t>(ch);
        /*
        // Convert ASCII character to the corresponding hex value
        if (ch >= '0' && ch <= '9') {
            hexValue = ch - '0';
        }
        else if (ch >= 'A' && ch <= 'F') {
            hexValue = ch - 'A' + 10;
        }
        else if (ch >= 'a' && ch <= 'f') {
            hexValue = ch - 'a' + 10;
        }
        else {
            std::cerr << "Invalid hexadecimal character: " << ch << std::endl;
            return; // Exit if invalid character
        }
        */
        // Shift combined left by 8 bits (1 BYTE) to make space, then add the new hex value 
        combined = (combined << 8) | hexValue;
    }



    // Update instr bitset<32> from the 32-bit integer
    instr = bitset<32>(toBigEndian(combined));

}

Controller::Controller(Instruction s)
{
    //Based on the opcode, find the type:

    //R-Type
    bitset<32> opcode_R(0b0110011);
    //I -Type
    bitset<32> opcode_I(0b0010011);
    //U-Type(LUI)
    bitset<32> opcode_U(0b0110111);
    //LOAD WORD/BYTE
    bitset<32> opcode_L(0b0000011);
    //STORE WORD/BYTE
    bitset<32> opcode_S(0b0100011);
    //J-Type(JAL)
    bitset<32> opcode_J(0b1101111);
    //B-Type (BEQ)
    bitset<32> opcode_B(0b1100011);

    //R-TYPE
    if ((s.instr & bitset<32>(0x7F)) == opcode_R) {
        regWrite = 1;
        AluSrc = 0;
        Branch = 0;
        MemRe = 0;
        MemWr = 0;
        MemtoReg = 0;

        //Find the conditions for ALUOp
        ALUOp = bitset<2>(0b10);

        //Opcode
        opcode = bitset<7>(0b0110011);
    }
    //I-TYPE
    else if (((s.instr & bitset<32>(0x7F)) == opcode_I)) {
        regWrite = 1;
        AluSrc = 1;
        Branch = 0;
        MemRe = 0;
        MemWr = 0;
        MemtoReg = 0;

        //Find the conditions for ALUOp
        ALUOp = bitset<2>(0b10);

        //Opcode
        opcode = bitset<7>(0b0010011);
    }
    //U-TYPE
    else if (((s.instr & bitset<32>(0x7F)) == opcode_U)) {
        regWrite = 1;
        AluSrc = 1;
        Branch = 0;
        MemRe = 0;
        MemWr = 0;
        MemtoReg = 0;

        //Find the conditions for ALUOp
        ALUOp = bitset<2>(0b10);

        //Opcode
        opcode = bitset<7>(0b0110111);
    }
    //LOAD
    else if (((s.instr & bitset<32>(0x7F)) == opcode_L)) {
        regWrite = 1;
        AluSrc = 1;
        Branch = 0;
        MemRe = 1;
        MemWr = 0;
        MemtoReg = 1;

        //Find the conditions for ALUOp
        ALUOp = bitset<2>(0b0);

        //Opcode
        opcode = bitset<7>(0b0000011);
    }
    //STORE
    else if (((s.instr & bitset<32>(0x7F)) == opcode_S)) {
        regWrite = 0;
        AluSrc = 1;
        Branch = 0;
        MemRe = 0;
        MemWr = 1;
        MemtoReg = 0;

        //Find the conditions for ALUOp
        ALUOp = bitset<2>(0b0);

        //Opcode
        opcode = bitset<7>(0b0100011);
    }
    else if (((s.instr & bitset<32>(0x7F)) == opcode_J)) {
        regWrite = 1;
        AluSrc = 0;
        Branch = 1;
        MemRe = 0;
        MemWr = 0;
        MemtoReg = 0;

        //Find the conditions for ALUOp
        ALUOp = bitset<2>(0b11);

        //Opcode
        opcode = bitset<7>(0b1101111);
    }
    else if (((s.instr & bitset<32>(0x7F)) == opcode_B)) {
        regWrite = 0;
        AluSrc = 0;
        Branch = 1;
        MemRe = 0;
        MemWr = 0;
        MemtoReg = 0;

        //Find the conditions for ALUOp
        ALUOp = bitset<2>(0b01);

        //Opcode
        opcode = bitset<7>(0b1100011);
    }

}


ALU_Controller::ALU_Controller(Instruction s, bitset<2> ALUOp_FrmController)
{
    // If it is R-Type/I-type/U-Type
    if (ALUOp_FrmController == bitset<2>(0b10)) {
        //Checking for FUNCT3 since only have to account for ADD and XOR (R-TYPE)
        bitset<32> maskFunct3(0x7000); // Just 1's in the Funct3 Section

        //If its ADD
        if ((maskFunct3 & s.instr) == bitset<32>(0)) {
            ALUOp = bitset<4>(0b0010);
        }
        //If its XOR
        else if ((maskFunct3 & s.instr) == bitset<32>(0x4000)) {
            ALUOp = bitset<4>(0b0011);
        }

        //Checking for FUNCT3 since only have to account for ORI and SRAI (I-TYPE)
        //If its ORI
        if ((maskFunct3 & s.instr) == bitset<32>(0x6000)) {
            ALUOp = bitset<4>(0b0001);
        }
        //If its SRAI
        else if ((maskFunct3 & s.instr) == bitset<32>(0x5000)) {
            ALUOp = bitset<4>(0b0100);
        }

        //CHECKING only OPCODE for LUI
        //U-Type(LUI)
        bitset<32> opcode_U(0b0110111);
        if (((s.instr & bitset<32>(0x7F)) == opcode_U)) {
            ALUOp = bitset<4>(0b1000);
        }
    }
    //If its LOAD/STORE
    else if (ALUOp_FrmController == bitset<2>(0b0)) {
        ALUOp = bitset<4>(0b0010);
    }
    //If its BRANCH (BEQ)
    else if (ALUOp_FrmController == bitset<2>(0b01)) {
        ALUOp = bitset<4>(0b0110);
    }
    //If its JUMP (JAL)
    else if (ALUOp_FrmController == bitset<2>(0b11)) {
        ALUOp = bitset<4>(0b1111);
    }
}

//////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////



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


int32_t ImmGen(Instruction s) {
    //Based on the opcode, find the type:
    //R-Type
    //bitset<32> opcode_R(0b0110011);
    //I -Type
    //bitset<32> opcode_I(0b0010011);
    //U-Type(LUI)
    //bitset<32> opcode_U(0b0110111);
    //LOAD WORD/BYTE
    bitset<32> opcode_L(0b0000011);
    //STORE WORD/BYTE
    bitset<32> opcode_S(0b0100011);
    //J-Type(JAL)
    bitset<32> opcode_J(0b1101111);

    //R-TYPE
    if ((s.instr & bitset<32>(0x7F)) == bitset<32>(0b0110011)) {
        return 0;
    }
    //I-TYPE
    else if (((s.instr & bitset<32>(0x7F)) == bitset<32>(0b0010011))) {
        //Checking for FUNCT3 since only have to account for ORI and SRAI (I-TYPE)
        bitset<32> maskFunct3(0x7000); // Just 1's in the Funct3 Section

        //If its ORI
        if ((maskFunct3 & s.instr) == bitset<32>(0x6000)) {
            bitset <12> top12Bits = (s.instr >> 20).to_ulong();

            // Convert to signed 12-bit integer
            int32_t signedValue;
            if (top12Bits[11]) { // If the most significant bit (sign bit) is 1
                signedValue = static_cast<int32_t>(top12Bits.to_ulong() | 0xFFFFF000);
            }
            else {
                signedValue = static_cast<int32_t>(top12Bits.to_ulong());
            }
            return signedValue;

        }
        //If its SRAI
        else if ((maskFunct3 & s.instr) == bitset<32>(0x5000)) {
            bitset<5> shiftbits = (s.instr >> 20).to_ulong();

            // Convert to signed 12-bit integer
            int32_t signedValue;
            if (shiftbits[4]) { // If the most significant bit (sign bit) is 1
                signedValue = static_cast<int32_t>(shiftbits.to_ulong() | 0xFFFFF000);
            }
            else {
                signedValue = static_cast<int32_t>(shiftbits.to_ulong());
            }
            return signedValue;
        }
    }
    //U-TYPE
    else if (((s.instr & bitset<32>(0x7F)) == bitset<32>(0b0110111))) {
        //If its LUI
        bitset <32> top20Bits = (s.instr & bitset<32>(0xFFFFF000));
        uint32_t unsignedVal = top20Bits.to_ulong();
        return static_cast<int32_t>(unsignedVal);

        // Convert to signed 32-bit integer
        // Check if the most significant bit (sign bit) is set
 /*     if (top20Bits[31]) { // If the sign bit is 1
            return static_cast<int32_t>(unsignedVal | 0xFFFFFFFF); // Apply two's complement
        }
        else {
            return static_cast<int32_t>(unsignedVal); // If sign bit is 0, it's already positive
        }
 */
    } 
    //LOAD
    else if (((s.instr & bitset<32>(0x7F)) == bitset<32>(0b0000011))) {
        bitset <12> top12Bits = (s.instr >> 20).to_ulong();

        // Convert to signed 12-bit integer
        int32_t signedValue;
        if (top12Bits[11]) { // If the most significant bit (sign bit) is 1
            signedValue = static_cast<int32_t>(top12Bits.to_ulong() | 0xFFFFF000);
        }
        else {
            signedValue = static_cast<int32_t>(top12Bits.to_ulong());
        }
        return signedValue;
    }
    //Store
    else if (((s.instr & bitset<32>(0x7F)) == bitset<32>(0b0100011))) {

        //Combining the 7-11 bits and the 25-31 bits to get the immediate
        int concatResult = (s.instr[31] << 6) | // 31
            (s.instr[30] << 5) | // 30
            (s.instr[29] << 4) | // 29
            (s.instr[28] << 3) | // 28
            (s.instr[27] << 2) | // 27
            (s.instr[26] << 1) | //26
            (s.instr[25]);       // 25

        concatResult <<= 5; // Shift left to make space for the next 5 bits

        concatResult |= (s.instr[11] << 4) | // 11
            (s.instr[10] << 3) | // 10
            (s.instr[9] << 2) | // 9
            (s.instr[8] << 1) | // 8
            (s.instr[7]);        // 7

        bitset <12> top12Bits(concatResult);

        // Convert to signed 12-bit integer
        int32_t signedValue;
        if (top12Bits[11]) { // If the most significant bit (sign bit) is 1
            signedValue = static_cast<int32_t>(top12Bits.to_ulong() | 0xFFFFF000);
        }
        else {
            signedValue = static_cast<int32_t>(top12Bits.to_ulong());
        }
        return signedValue;

    }
    //BRANCH
    else if (((s.instr & bitset<32>(0x7F)) == bitset<32>(0b1100011))) {

       //Combining the 7-11 bits and the 25-31 bits to get the immediate
       int concatResult = (s.instr[31] << 12) | // 12
           (s.instr[30] << 10) | // 10
           (s.instr[29] << 9) | // 9
           (s.instr[28] << 8) | // 8
           (s.instr[27] << 7) | // 7
           (s.instr[26] << 6) | //6
           (s.instr[25] << 5) |  //5
           (s.instr[7] << 11) | // 11
           (s.instr[11] << 4) | // 4
           (s.instr[10] << 3) | // 3
           (s.instr[9] << 2) | // 2
           (s.instr[8] << 1) | //1
           0;  //0 NEED TO BE ZEROES DOUT

        bitset <13> top13Bits(concatResult);

        // Convert to signed 12-bit integer
        int32_t signedValue;
        if (top13Bits[12]) { // If the most significant bit (sign bit) is 1
            signedValue = static_cast<int32_t>(top13Bits.to_ulong() | 0xFFFFF000);
        }
        else {
            signedValue = static_cast<int32_t>(top13Bits.to_ulong());
        }
        return signedValue;

    }    
    //JUMP
    else if (((s.instr & bitset<32>(0x7F)) == bitset<32>(0b1101111))) {

       //Combining the top 20 bits to get the immediate
       int concatResult = (s.instr[31] << 20) | // 20
           (s.instr[30] << 10) | // 10
           (s.instr[29] << 9) | // 9
           (s.instr[28] << 8) | // 8
           (s.instr[27] << 7) | // 7
           (s.instr[26] << 6) | //6
           (s.instr[25] << 5) |  //5
           (s.instr[24] << 4) |  //4
           (s.instr[23] << 3) |  //3
           (s.instr[22] << 2) |  //2
           (s.instr[21] << 1) |  //1
           (s.instr[20] << 11) | // 11
           (s.instr[19] << 19) | // 19
           (s.instr[18] << 18) | // 18
           (s.instr[17] << 17) | // 17
           (s.instr[16] << 16) | // 16
           (s.instr[15] << 15) | // 15
           (s.instr[14] << 14) | // 14
           (s.instr[13] << 13) | // 13
           (s.instr[12] << 12) | // 12
           0;  //0 NEED TO BE ZEROES DOUT

        bitset <21> top21Bits(concatResult);

        // Convert to signed 12-bit integer
        int32_t signedValue;
        if (top21Bits[20]) { // If the most significant bit (sign bit) is 1
            signedValue = static_cast<int32_t>(top21Bits.to_ulong() | 0xFFFFF000);
        }
        else {
            signedValue = static_cast<int32_t>(top21Bits.to_ulong());
        }
        return signedValue;

    }
    else {
        return 0;
    }
    //Should not reach here
    return 0;    
}

int32_t ALU_Result(int x1, int x2, bitset<4> ALUOp)
{
    //ADD
    if (ALUOp == bitset<4>(0b0010)) {
        return x1 + x2;
    }
    //SUBTRACT//BEQ
    else if (ALUOp == bitset<4>(0b0110)) {
        return x1 - x2;
    }
    //AND
    else if (ALUOp == bitset<4>(0b0000)) {
        return x1 & x2;
    }
    //ORI
    else if (ALUOp == bitset<4>(0b0001)) {
        return x1 | x2;
    }
    //XOR
    else if (ALUOp == bitset<4>(0b0011)) {
        return x1 ^ x2;
    }
    //SRAI
    else if (ALUOp == bitset<4>(0b0100)) {
        return x1 >> x2;
    }
    //LUI
    else if (ALUOp == bitset<4>(0b1000)) {
        return x2;
    }
    //JAL
    else if (ALUOp == bitset<4>(0b1111)) {
    //RETURN 0
        return 0;
    }
    else {
        return 0;
    }
    
}

int32_t CPU::DataMemory(int MemWrite, int MemRead, int ALUResult, int rs2, bool word)
{
    if (MemWrite && word) {
        dmemory[ALUResult / 4] = rs2;
        return 0;
    }
    //IF the store is just a byte
    else if (MemWrite && !(word) ) {
        dmemory[ALUResult / 4] = rs2 & 0xFF;
        return 0;
    }
    else if (MemRead && word) {
        return dmemory[ALUResult / 4];
    }
    //IF the load is just a byte
    else if (MemRead && !(word)) {
        return (dmemory[ALUResult / 4] & 0xFF);
    }
    else
        return -1;
}

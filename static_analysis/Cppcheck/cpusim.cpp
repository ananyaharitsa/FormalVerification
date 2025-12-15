#include "CPU.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char* argv[])
{
    unsigned char instMem[4096] = {0};

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <trace-file>\n";
        return -1;
    }

    std::ifstream infile(argv[1]);
    if (!infile.is_open()) {
        std::cerr << "error opening file " << argv[1] << "\n";
        return -1;
    }

    std::string line;
    int i = 0;
    while (std::getline(infile, line) && i < 4096) {
        std::stringstream line2(line);
        int hexValue = 0;
        line2 >> std::hex >> hexValue;
        instMem[i] = static_cast<unsigned char>(hexValue & 0xFF);
        ++i;
    }
    int maxPC = i;

    CPU myCPU;
    const int NUM_REGISTERS = 32;
    int registers[NUM_REGISTERS] = {0};

    // explicit loop with clear break conditions
    while (true) {
        // if not enough bytes left for an instruction, stop
        if ((i - static_cast<int>(myCPU.readPC())) < 4) break;

        Instruction myInst(instMem, myCPU);
        unsigned long nextPC = myCPU.readPC() + 4;

        Controller myController(myInst);
        ALU_Controller myALU_Control(myInst, myController.ALUOp);
        int32_t ImmValue = ImmGen(myInst);

        int rd = static_cast<int>((myInst.instr[11] << 4) | (myInst.instr[10] << 3) | (myInst.instr[9] << 2) | (myInst.instr[8] << 1) | (myInst.instr[7]));
        int rs1 = static_cast<int>((myInst.instr[19] << 4) | (myInst.instr[18] << 3) | (myInst.instr[17] << 2) | (myInst.instr[16] << 1) | (myInst.instr[15]));
        int rs2 = static_cast<int>((myInst.instr[24] << 4) | (myInst.instr[23] << 3) | (myInst.instr[22] << 2) | (myInst.instr[21] << 1) | (myInst.instr[20]));

        // ensure valid indices
        if (rd < 0 || rd >= NUM_REGISTERS) rd = 0;
        if (rs1 < 0 || rs1 >= NUM_REGISTERS) rs1 = 0;
        if (rs2 < 0 || rs2 >= NUM_REGISTERS) rs2 = 0;

        registers[0] = 0;
        int rs1Val = registers[rs1];
        int rs2Val = registers[rs2];
        int prevRS2 = rs2Val;
        rs2Val = myController.AluSrc ? ImmValue : rs2Val;

        int32_t ALU_Res = ALU_Result(rs1Val, rs2Val, myALU_Control.ALUOp);
        bool zeroFlag = (ALU_Res == 0);

        if ((myController.Branch == zeroFlag) && zeroFlag) {
            nextPC = myCPU.readPC() + ImmValue;
        }

        bool isWord = false;
        if (myController.opcode == std::bitset<7>(0b0000011)) {
            if ((myInst.instr & std::bitset<32>(0x7000)) == std::bitset<32>(0x2000)) isWord = true;
            else isWord = false;
        } else if (myController.opcode == std::bitset<7>(0b0100011)) {
            if ((myInst.instr & std::bitset<32>(0x7000)) == std::bitset<32>(0x2000)) isWord = true;
            else isWord = false;
        }

        int32_t Read_Data = myCPU.DataMemory(myController.MemWr, myController.MemRe, ALU_Res, prevRS2, isWord);

        if (myController.regWrite) {
            if (myController.opcode == std::bitset<7>(0b1101111)) {
                if (rd >= 0 && rd < NUM_REGISTERS) registers[rd] = static_cast<int>(myCPU.readPC() + 4);
            } else {
                if (rd >= 0 && rd < NUM_REGISTERS) registers[rd] = myController.MemtoReg ? Read_Data : ALU_Res;
            }
        }

        myCPU.incPC(nextPC);
        if (myCPU.readPC() > static_cast<unsigned long>(maxPC)) break;
    }

    int a0 = registers[10];
    int a1 = registers[11];
    std::cout << "(" << a0 << "," << a1 << ")" << std::endl;
    return 0;
}

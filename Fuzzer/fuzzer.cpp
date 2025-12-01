#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include "../CPU_Files/CPU.h" // CPU class and functions

using namespace std;

// Generate a random 32-bit instruction (4 bytes)
vector<unsigned char> generateRandomInstruction() {
    vector<unsigned char> inst(4);
    for (int i = 0; i < 4; ++i) {
        inst[i] = rand() % 256;
    }
    return inst;
}

void saveInstructions(const vector<unsigned char> &instructions, const string &filename) {
    ofstream out(filename);
    for (auto byte : instructions) {
        out << hex << (int)byte << endl;
    }
    out.close();
}

// Run the CPU simulator on a set of instructions
void runCPU(vector<unsigned char> &instructions) {
    CPU myCPU;
    const int NUM_REGISTERS = 32;
    int registers[NUM_REGISTERS] = {0};

    size_t maxPC = instructions.size();
    while (myCPU.readPC() + 4 <= maxPC) {
        Instruction myInst(instructions.data(), myCPU);
        Controller myController(myInst);
        ALU_Controller myALU_Control(myInst, myController.ALUOp);
        int32_t ImmValue = ImmGen(myInst);

        int rd = (myInst.instr[11] << 4) | (myInst.instr[10] << 3) |
                 (myInst.instr[9] << 2) | (myInst.instr[8] << 1) |
                 (myInst.instr[7]);
        int rs1 = (myInst.instr[19] << 4) | (myInst.instr[18] << 3) |
                  (myInst.instr[17] << 2) | (myInst.instr[16] << 1) |
                  (myInst.instr[15]);
        int rs2 = (myInst.instr[24] << 4) | (myInst.instr[23] << 3) |
                  (myInst.instr[22] << 2) | (myInst.instr[21] << 1) |
                  (myInst.instr[20]);

        registers[0] = 0;
        int rs1Val = registers[rs1];
        int rs2Val = myController.AluSrc ? ImmValue : registers[rs2];
        int32_t ALU_Res = ALU_Result(rs1Val, rs2Val, myALU_Control.ALUOp);
        bool zeroFlag = ALU_Res ? 0 : 1;

        unsigned long nextPC = myCPU.readPC() + 4;
        if ((myController.Branch == zeroFlag) && (zeroFlag == 1)) {
            nextPC = myCPU.readPC() + ImmValue;
        }

        int32_t Read_Data = myCPU.DataMemory(myController.MemWr, myController.MemRe, ALU_Res, registers[rs2], true);

        if (myController.regWrite) {
            registers[rd] = myController.MemtoReg ? Read_Data : ALU_Res;
        }

        myCPU.incPC(nextPC);
    }

    cout << "Final a0, a1: (" << registers[10] << "," << registers[11] << ")" << endl;
}

int main() {
    const int NUM_INSTRUCTIONS = 10000; // Number of random instructions
    vector<unsigned char> instructions;

    for (int i = 0; i < NUM_INSTRUCTIONS; ++i) {
        vector<unsigned char> inst = generateRandomInstruction();
        instructions.insert(instructions.end(), inst.begin(), inst.end());
    }

    runCPU(instructions);
    saveInstructions(instructions, "Test/fuzzer_trace/random_inst.txt");
    return 0;
}

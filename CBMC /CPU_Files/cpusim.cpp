// cpusim.cpp (Revised for CBMC Compatibility)
#include "CPU.h" 
// Only include essential C++ I/O headers for the main program's utility (file I/O, string stream)
// CBMC will tolerate these if the included headers are minimal, but we need to remove 'using namespace std'
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdio.h> // For standard C types

// Removed: using namespace std;

int main(int argc, char* argv[])
{
    /* This is the front end of your project. */
    
    unsigned char instMem[4096];

    if (argc < 2) {
        // We use std::cout and std::endl since we removed 'using namespace std'
        // std::cout << "No file name entered. Exiting..." << std::endl;
        return -1;
    }

    std::ifstream infile(argv[1]); //open the file
    if (!(infile.is_open() && infile.good())) {
        std::cout << "error opening file\n";
        return 0;
    }

    std::string line;
    int i = 0;
    
    // Read each line in the input file, assuming each line represents 1 byte in hexadecimal
    while (std::getline(infile, line) && i < 4096) {
        std::stringstream line2(line);
        int hexValue;

        // Convert each hex line to an integer, then cast it to char and store in instMem
        line2 >> std::hex >> hexValue;
        instMem[i] = static_cast<char>(hexValue);

        i++;
    }
    int maxPC = i;


    CPU myCPU;  
    
    //REGISTERS and their values (All set to zero to start)
    const int NUM_REGISTERS = 32;
    int registers[NUM_REGISTERS] = { 0 };


    bool done = true;
    while (done == true) // processor's main loop. Each iteration is equal to one clock cycle.  
    {
        ///////////////
        //// FETCH ////
        ///////////////

        //Only make an instruction if its 32 more bits
        if (i - myCPU.readPC() < 4)
            break;
        
        /* Instantiate your Instruction object here. */
        Instruction myInst(instMem, myCPU); 

        //Getting the next PC without jumps
        unsigned long nextPC = myCPU.readPC() + 4;

        ////////////////
        //// DECODE ////
        ////////////////
        Controller myController(myInst);
        ALU_Controller myALU_Control(myInst, myController.ALUOp);
        int32_t ImmValue = ImmGen(myInst);

        //Register Address 
        // NOTE: In the original code, myInst.instr was a bitset<32>, allowing array access [i].
        // Assuming the Instruction class was updated to use uint32_t (myInst.instr), 
        // we extract the fields using bitwise operations directly on the 32-bit integer.

        uint32_t instr_val = myInst.instr; // Use the raw 32-bit integer

        // rd (bits 11-7)
        int rd = (instr_val >> 7) & 0x1F;
        // rs1 (bits 19-15)
        int rs1 = (instr_val >> 15) & 0x1F;
        // rs2 (bits 24-20)
        int rs2 = (instr_val >> 20) & 0x1F;


        ////////////////
        // EXECUTION  //
        ////////////////

        //Keeping x0 at 0
        registers[0] = 0;
        
        //Read Registers
        int rs1Val = registers[rs1];
        int rs2Val = registers[rs2];

        //KEEP THIS IN CASE OF WRITE DATAPATH
        int prevRS2 = rs2Val;

        //MUX1 Between RS2 and Imm Gen going into ALU
        rs2Val = myController.AluSrc ? ImmValue : rs2Val;

        //ALU Operation
        // ALUOp is now a uint8_t
        int32_t ALU_Res = ALU_Result(rs1Val, rs2Val, myALU_Control.ALUOp);
        bool zeroFlag = ALU_Res ? 0 : 1;
        
        //Check on Branch Condition (Changes the next PC to jump)
        if ((myController.Branch == zeroFlag) && (zeroFlag == 1)) {
            nextPC = myCPU.readPC() + ImmValue; 
        }

        /////////////////
        //MEMORY ACCESS//
        /////////////////

        //If the LOAD/Store
        bool isWord = false;

        // Constants used to check Funct3 field (bits 14-12)
        const uint32_t FUNCT3_MASK = 0x7000;
        const uint32_t FUNCT3_WORD = 0x2000; // 010
        const uint32_t FUNCT3_BYTE = 0x0000; // 000
        
        // Opcode is now a uint8_t
        const uint8_t OPCODE_LOAD = 0x03;
        const uint8_t OPCODE_STORE = 0x23;


        //If we write from memory (LOAD)
        if (myController.opcode == OPCODE_LOAD) {
            //LOAD WORD
            if ((instr_val & FUNCT3_MASK) == FUNCT3_WORD) {
                isWord = true;
            }
            //LOAD BYTE
            else if ((instr_val & FUNCT3_MASK) == FUNCT3_BYTE) {
                isWord = false;
            }
        }
        //If we write to memory (STORE)
        else if (myController.opcode == OPCODE_STORE) {
            //STORE WORD (32 bits)
            if ((instr_val & FUNCT3_MASK) == FUNCT3_WORD) {
                isWord = true;
            }
            //STORE BYTE
            else if ((instr_val & FUNCT3_MASK) == FUNCT3_BYTE) {
                isWord = false;
            }
        }

        // DATA MEMORY OUTPUT
        int32_t Read_Data = myCPU.DataMemory(myController.MemWr, myController.MemRe, ALU_Res, prevRS2, isWord);

        //////////////
        //WRITE BACK//
        //////////////
        
        const uint8_t OPCODE_JAL = 0x6F; // 1101111
        
        if (myController.regWrite) {
            //ADD A condition to Write the next PC if its a JAL
            if (myController.opcode == OPCODE_JAL) {
                registers[rd] = myCPU.readPC() + 4;
            }
            else {
                registers[rd] = myController.MemtoReg ? Read_Data : ALU_Res;
            }
        }


        //Update PC 
        myCPU.incPC(nextPC);
        if (myCPU.readPC() > maxPC)
            break;
    }


    int a0 = registers[10];
    int a1 = registers[11];
    // print the results (using std::cout and std::endl)
    std::cout << "(" << a0 << "," << a1 << ")" << std::endl;

    return 0;

}
#include "CPU.h"

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include<fstream>
#include <sstream>
#include<vector>
using namespace std;

/*
Add all the required standard and developed libraries here
*/

/*
Put/Define any helper function/definitions you need here
*/





int main(int argc, char* argv[])
{
	/* This is the front end of your project.
	You need to first read the instructions that are stored in a file and load them into an instruction memory.
	*/

	/* Each cell should store 1 byte. You can define the memory either dynamically, or define it as a fixed size with size 4KB (i.e., 4096 lines). Each instruction is 32 bits (i.e., 4 lines, saved in little-endian mode).
	Each line in the input file is stored as an hex and is 1 byte (each four lines are one instruction). You need to read the file line by line and store it into the memory. You may need a mechanism to convert these values to bits so that you can read opcodes, operands, etc.
	*/
	unsigned char instMem[4096];


	if (argc < 2) {
		//cout << "No file name entered. Exiting...";
		return -1;
	}

	ifstream infile(argv[1]); //open the file
	if (!(infile.is_open() && infile.good())) {
		cout << "error opening file\n";
		return 0;
	}

	string line;
	int i = 0;
	/*while (infile) {
		infile >> line;
		stringstream line2(line);
		char x;
		line2 >> x;
		instMem[i] = x; // be careful about hex
		i++;
		line2 >> x;
		instMem[i] = x; // be careful about hex
		//cout << instMem[i] << endl;
		i++;

	}
	int maxPC = i;
	*/
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


	/* Instantiate your CPU object here.  CPU class is the main class in this project that defines different components of the processor.
	CPU class also has different functions for each stage (e.g., fetching an instruction, decoding, etc.).
	*/

	CPU myCPU;  // call the approriate constructor here to initialize the processor...  
	// make sure to create a variable for PC and resets it to zero (e.g., unsigned int PC = 0); 

	//REGISTERS and their values (All set to zero to start)
	const int NUM_REGISTERS = 32;
	int registers[NUM_REGISTERS] = { 0 };



	bool done = true;
	while (done == true) // processor's main loop. Each iteration is equal to one clock cycle.  
	{
		//fetch
		
		//Only make an instruction if its 32 more bits
		if (i - myCPU.readPC() < 4)
			break;

		/* Instantiate your Instruction object here. */
		Instruction myInst(instMem, myCPU); 

		//Getting the next PC without jumps
		unsigned long nextPC = myCPU.readPC() + 4;

		// decode
		Controller myController(myInst);
		ALU_Controller myALU_Control(myInst, myController.ALUOp);
		int32_t ImmValue = ImmGen(myInst);

		//Register Address 
		int rd =  (myInst.instr[11] << 4) | (myInst.instr[10] << 3) | (myInst.instr[9] << 2)  |	(myInst.instr[8] << 1)  | (myInst.instr[7]);	
		int rs1 = (myInst.instr[19] << 4) | (myInst.instr[18] << 3) | (myInst.instr[17] << 2) | (myInst.instr[16] << 1) | (myInst.instr[15]);
		int rs2 = (myInst.instr[24] << 4) | (myInst.instr[23] << 3) | (myInst.instr[22] << 2) | (myInst.instr[21] << 1) | (myInst.instr[20]);

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
		int32_t ALU_Res = ALU_Result(rs1Val, rs2Val, myALU_Control.ALUOp);
		bool zeroFlag = ALU_Res ? 0 : 1;
		
		//Check on Branch Condition (Changes the next PC to jump)
		if ((myController.Branch == zeroFlag) && (zeroFlag == 1)) {
			nextPC = myCPU.readPC() + ImmValue; //Only multiplied by 4 to compensate                        *CHECK THIS PART*
		}


		/////////////////
		//MEMORY STUFF
		/////////////////

		//If the LOAD/Store
		bool isWord = false;

		//If we write from memory (LOAD)
		if (myController.opcode == bitset<7>(0b0000011)) {
			//LOAD WORD
			if ((myInst.instr & bitset<32>(0x7000)) == bitset<32>(0x2000)) {
				isWord = true;
			}
			//LOAD BYTE
			if ((myInst.instr & bitset<32>(0x7000)) == bitset<32>(0b0)) {
				isWord = false;
			}
		}
		//If we write to memory (STORE)
		else if (myController.opcode == bitset<7>(0b0100011)) {
			//STORE WORD (32 bits)
			if ((myInst.instr & bitset<32>(0x7000)) == bitset<32>(0x2000)) {
				isWord = true;
			}
			//STORE BYTE
			if ((myInst.instr & bitset<32>(0x7000)) == bitset<32>(0b0)) {
				isWord = false;
			}
		}

		// DATA MEMORY OUTPUT
		int32_t Read_Data = myCPU.DataMemory(myController.MemWr, myController.MemRe, ALU_Res, prevRS2, isWord);


		//WRITING BACK TO REGISTER
		if (myController.regWrite) {
			//ADD A condition to Write the next PC if its a JAL
			if (myController.opcode == bitset<7>(0b1101111)) {
				registers[rd] = myCPU.readPC() + 4;
			}
			else {
				registers[rd] = myController.MemtoReg ? Read_Data : ALU_Res;
			}
		}


		// ... 
		myCPU.incPC(nextPC);
		if (myCPU.readPC() > maxPC)
			break;
	}



	int a0 = registers[10];
	int a1 = registers[11];
	// print the results (you should replace a0 and a1 with your own variables that point to a0 and a1)
	cout << "(" << a0 << "," << a1 << ")" << endl;

	return 0;

}
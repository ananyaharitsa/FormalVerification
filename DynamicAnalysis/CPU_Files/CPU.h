#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;


class CPU {
private:
	int dmemory[4096]; //data memory byte addressable in little endian fashion;
	unsigned long PC; //pc 

public:
	CPU();

	// SAFETY FOR FUZZING
    bool errorFlag = false;          // True if a crash occurred
    string errorMessage = "";        // Details about the crash
   
	unsigned long readPC();
	void incPC(unsigned long nextPC);
	int32_t DataMemory(int MemWrite, int MemRead, int ALUResult, int rs2, bool word);

};

class Instruction { // optional
public:
	bitset<32> instr = 0;//instruction
	Instruction(unsigned char instructionMem[], CPU cpu); // constructor
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



// add other functions and objects here
#pragma once

uint32_t toBigEndian(uint32_t value);
int32_t ImmGen(Instruction s);
int32_t ALU_Result(int x1, int x2, bitset<4> ALUOp);

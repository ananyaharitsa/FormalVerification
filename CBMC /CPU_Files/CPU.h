// CPU.h (Final CBMC Ready Version)

// Use C-style headers for basic types/functions
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// Removed: #include <bitset>

class CPU {
private:
	int dmemory[4096]; //data memory byte addressable in little endian fashion;
	unsigned long PC; //pc 

public:
    // ... (rest of CPU class is unchanged) ...
	CPU();
	unsigned long readPC();
	void incPC(unsigned long nextPC);
	int32_t DataMemory(int MemWrite, int MemRead, int ALUResult, int rs2, bool word);
};

class Instruction { 
public:
    // **CRITICAL CHANGE**: Use 32-bit unsigned integer instead of std::bitset<32>
	uint32_t instr = 0; 
	Instruction(unsigned char instructionMem[], CPU cpu); 
};


class Controller {

public:
	Controller(Instruction instr);
    // Explicitly using 0 initialization 
	bool regWrite = 0, AluSrc = 0, Branch = 0, MemRe = 0, MemWr = 0, MemtoReg = 0;
    // **CRITICAL CHANGE**: Use small unsigned integers (uint8_t, unsigned char) for small bitsets
    // These behave like fixed-size integers and are CBMC friendly.
	uint8_t ALUOp; // Replaces bitset<2>
	uint8_t opcode; // Replaces bitset<7>
};

class ALU_Controller {
public:
	ALU_Controller(Instruction s, uint8_t ALUOp_FrmController);
	uint8_t ALUOp; // Replaces bitset<4>
};


// add other functions and objects here
#pragma once

uint32_t toBigEndian(uint32_t value);
int32_t ImmGen(Instruction s);
// **CRITICAL CHANGE**: ALU_Result now takes uint8_t for ALUOp
int32_t ALU_Result(int x1, int x2, uint8_t ALUOp);
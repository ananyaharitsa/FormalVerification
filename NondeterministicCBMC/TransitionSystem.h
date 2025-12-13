#ifndef TRANSITION_SYSTEM_H
#define TRANSITION_SYSTEM_H

#include "CPU.h"
#include <vector>

class TransitionSystem {
public:
    // Inputs to try when reading from MMIO_INPUT_ADDR (0x4000)
    const std::vector<int> INTERESTING_INPUTS = {0, 1, 42, 100, -1}; 

    std::vector<CPU::StateSnapshot> GetNextStates(const CPU::StateSnapshot& current, unsigned char* instMem, int maxPC) {
        std::vector<CPU::StateSnapshot> next_states;
        
        CPU myCPU;
        myCPU.RestoreState(current);
        
        unsigned long pc = myCPU.readPC();
        // TERMINATION CHECK
        if (pc >= maxPC * 4 || pc >= 4096) return next_states; 

        // 1. FETCH & DECODE
        Instruction myInst(instMem, myCPU);
        Controller myCtrl(myInst);
        ALU_Controller myALU(myInst, myCtrl.ALUOp);
        int32_t ImmVal = ImmGen(myInst);

        int rs1 = (myInst.instr.to_ulong() >> 15) & 0x1F;
        int rs2 = (myInst.instr.to_ulong() >> 20) & 0x1F;
        int rd = (myInst.instr.to_ulong() >> 7) & 0x1F;

        myCPU.registers[0] = 0;
        int rs1Val = myCPU.registers[rs1];
        int rs2Val = myCPU.registers[rs2];
        int rs2Val_mux = myCtrl.AluSrc ? ImmVal : rs2Val;
        
        int32_t ALU_Res = ALU_Result(rs1Val, rs2Val_mux, myALU.ALUOp);

        // 2. DETECT MMIO READ (Non-Determinism Type 1)
        bool isInputRead = (myCtrl.MemRe && ALU_Res == CPU::MMIO_INPUT_ADDR);

        if (isInputRead) {
            // FORK 1: Create a state for each possible input
            for (int input_val : INTERESTING_INPUTS) {
                CPU forkCPU = myCPU;
                if (myCtrl.regWrite && rd != 0) {
                    forkCPU.registers[rd] = input_val; 
                }
                forkCPU.incPC(pc + 4);
                next_states.push_back(forkCPU.GetState());
            }
        } 
        else {
            // NORMAL EXECUTION
            CPU normalCPU = myCPU;
            bool isWord = ((myInst.instr.to_ulong() & 0x7000) == 0x2000);
            int32_t memData = 0;
            
            // Execute Memory Access safely
            if (myCtrl.MemRe || myCtrl.MemWr) {
                // Bounds check happens in VerifyState, here we just run logic
                if (ALU_Res >= 0 && ALU_Res < 4096 * 4) {
                    memData = normalCPU.DataMemory(myCtrl.MemWr, myCtrl.MemRe, ALU_Res, rs2Val, isWord);
                }
            }

            // Execute Writeback
            unsigned long nextPC = pc + 4;
            if (myCtrl.regWrite && rd != 0) {
                if (myCtrl.opcode == 0x6F) normalCPU.registers[rd] = pc + 4;
                else normalCPU.registers[rd] = myCtrl.MemtoReg ? memData : ALU_Res;
            }

            // Execute Branch
            bool zero = (ALU_Res == 0);
            if (myCtrl.Branch && zero) nextPC = pc + ImmVal;
            
            normalCPU.incPC(nextPC);
            next_states.push_back(normalCPU.GetState());
        }

        // 3. INTERRUPT FORK (Non-Determinism Type 2)
        // Check if interrupts are enabled (Mock: bit 0 of MSTATUS_REG x12)
        bool interruptsEnabled = (myCPU.registers[CPU::MSTATUS_REG] & 0x1);

        if (interruptsEnabled) {
            CPU interruptCPU = myCPU;
            
            // Save current PC to EPC (x30)
            interruptCPU.registers[CPU::EPC_REG] = pc; 
            
            // Disable Interrupts (Clear bit in MSTATUS x12)
            interruptCPU.registers[CPU::MSTATUS_REG] &= ~0x1; 
            
            // Jump to Handler
            interruptCPU.PC = CPU::ISR_HANDLER_ADDR; 
            
            next_states.push_back(interruptCPU.GetState());
        }

        return next_states;
    }
};

#endif
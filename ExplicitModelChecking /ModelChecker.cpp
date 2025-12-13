#include "CPU.h"
#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <fstream>
#include <sstream>

// Custom Hash Function for StateSnapshot
struct StateHash {
    size_t operator()(const CPU::StateSnapshot& s) const {
        size_t h1 = std::hash<unsigned long>{}(s.pc);
        size_t h2 = 0;
        for (int i = 0; i < 32; i++) {
            h2 ^= std::hash<int>{}(s.regs[i]) + 0x9e3779b9 + (h2 << 6) + (h2 >> 2);
        }
        // Hashing memory (simplified to first 100 words for performance, can hash all 4096 for correctness)
        for (int i = 0; i < 100; i++) {
             h2 ^= std::hash<int>{}(s.memory[i]) + 0x9e3779b9 + (h2 << 6) + (h2 >> 2);
        }
        return h1 ^ (h2 << 1);
    }
};

// --- PROPERTY CHECKER ---
bool VerifyState(CPU& cpu, Instruction& instr, Controller& ctrl, int32_t alu_res) {
    // 1. SAFETY: Memory Bounds & Alignment
    if (ctrl.MemRe || ctrl.MemWr) {
        if (alu_res < 0 || alu_res >= 4096 * 4) {
            std::cerr << "[FAIL] Memory Access Out of Bounds. PC: " << cpu.readPC() << " Addr: " << alu_res << std::endl;
            return false;
        }
        bool isWord = ((instr.instr.to_ulong() & 0x7000) == 0x2000); 
        if (isWord && (alu_res % 4 != 0)) {
            std::cerr << "[FAIL] Misaligned Word Access. PC: " << cpu.readPC() << " Addr: " << alu_res << std::endl;
            return false;
        }
    }
    // 2. INVARIANT: x0 is always 0
    if (cpu.registers[0] != 0) {
        std::cerr << "[FAIL] Register x0 corruption. PC: " << cpu.readPC() << std::endl;
        return false;
    }
    return true;
}


// --- BFS SEARCH (with Liveness Check) ---
void RunBFS(unsigned char* instMem, int maxPC) {
    CPU myCPU;
    std::queue<CPU::StateSnapshot> q;
    std::unordered_set<CPU::StateSnapshot, StateHash> visited;

    CPU::StateSnapshot initial = myCPU.GetState();
    q.push(initial);
    visited.insert(initial);

    int states_explored = 0;
    bool valid_termination_found = false; // <--- NEW TRACKER

    while (!q.empty()) {
        CPU::StateSnapshot current_snap = q.front();
        q.pop();
        states_explored++;

        myCPU.RestoreState(current_snap);
        unsigned long pc = myCPU.readPC();

        // LIVENESS CHECK: Did we successfully finish the program?
        // In this sim, "finish" means PC goes past the last instruction.
        if (pc >= maxPC * 4) {
            valid_termination_found = true; 
            continue; // Stop exploring this path, it succeeded.
        }

        Instruction myInst(instMem, myCPU);
        unsigned long nextPC = pc + 4;

        Controller myController(myInst);
        ALU_Controller myALU_Control(myInst, myController.ALUOp);
        int32_t ImmValue = ImmGen(myInst);

        // ... (Register Decoding & Execution Logic remains same) ...
        int rs1 = (myInst.instr.to_ulong() >> 15) & 0x1F;
        int rs2 = (myInst.instr.to_ulong() >> 20) & 0x1F;
        int rd = (myInst.instr.to_ulong() >> 7) & 0x1F;
        myCPU.registers[0] = 0;
        int rs1Val = myCPU.registers[rs1];
        int rs2Val = myCPU.registers[rs2];
        int rs2Val_mux = myController.AluSrc ? ImmValue : rs2Val;
        int32_t ALU_Res = ALU_Result(rs1Val, rs2Val_mux, myALU_Control.ALUOp);
        
        // Safety Property Verification
        if (!VerifyState(myCPU, myInst, myController, ALU_Res)) return;

        // ... (Memory & Writeback Logic remains same) ...
        bool zeroFlag = (ALU_Res == 0);
        if (myController.Branch && zeroFlag) nextPC = pc + ImmValue;
        bool isWord = ((myInst.instr.to_ulong() & 0x7000) == 0x2000);
        int32_t Read_Data = myCPU.DataMemory(myController.MemWr, myController.MemRe, ALU_Res, rs2Val, isWord);
        if (myController.regWrite && rd != 0) {
            if (myController.opcode == 0x6F) myCPU.registers[rd] = pc + 4;
            else myCPU.registers[rd] = myController.MemtoReg ? Read_Data : ALU_Res;
        }

        myCPU.incPC(nextPC);

        // Add next state to queue
        CPU::StateSnapshot next_state = myCPU.GetState();
        if (visited.find(next_state) == visited.end()) {
            visited.insert(next_state);
            q.push(next_state);
        }
    }

    // --- FINAL REPORT ---
    if (!valid_termination_found) {
        std::cout << "[FAIL] Liveness Violation: Program never terminates (Infinite Loop detected)." << std::endl;
    } else {
        std::cout << ">>> VERIFICATION SUCCESSFUL! Program terminates safely." << std::endl;
    }
    std::cout << "States Explored: " << states_explored << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./modelchecker <instruction_file>" << std::endl;
        return -1;
    }
    unsigned char instMem[4096] = {0};
    std::ifstream infile(argv[1]);
    if (!infile.is_open()) { std::cout << "Error opening file\n"; return 0; }

    std::string line;
    int i = 0;
    while (std::getline(infile, line) && i < 4096) {
        std::stringstream line2(line);
        int hexValue;
        line2 >> std::hex >> hexValue;
        instMem[i] = static_cast<char>(hexValue);
        i++;
    }
    RunBFS(instMem, i / 4); // Pass roughly instruction count
    return 0;
}
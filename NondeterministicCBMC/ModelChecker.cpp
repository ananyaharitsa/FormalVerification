#include "CPU.h"
#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <fstream>
#include <sstream>

// Custom Hash
struct StateHash {
    size_t operator()(const CPU::StateSnapshot& s) const {
        size_t h1 = std::hash<unsigned long>{}(s.pc);
        size_t h2 = 0;
        for (int i = 0; i < 32; i++) h2 ^= std::hash<int>{}(s.regs[i]) + 0x9e3779b9 + (h2 << 6) + (h2 >> 2);
        for (int i = 0; i < 100; i++) h2 ^= std::hash<int>{}(s.memory[i]) + 0x9e3779b9 + (h2 << 6) + (h2 >> 2);
        return h1 ^ (h2 << 1);
    }
};

bool VerifyState(CPU& cpu, Instruction& instr, Controller& ctrl, int32_t alu_res) {
    if (ctrl.MemRe || ctrl.MemWr) {
        if (alu_res < 0 || alu_res >= 4096 * 4) {
            if (alu_res != CPU::MMIO_INPUT_ADDR) {
                std::cerr << "[FAIL] Memory Violation. PC: " << cpu.readPC() << " Addr: " << alu_res << std::endl;
                return false;
            }
        }
        // Updated: Access raw instr directly
        bool isWord = ((instr.instr & 0x7000) == 0x2000); 
        if (isWord && (alu_res % 4 != 0)) {
            std::cerr << "[FAIL] Misalignment. PC: " << cpu.readPC() << " Addr: " << alu_res << std::endl;
            return false;
        }
    }
    return true;
}

void RunBFS(unsigned char* instMem, int maxPC) {
    CPU myCPU;
    std::queue<CPU::StateSnapshot> q;
    std::unordered_set<CPU::StateSnapshot, StateHash> visited;

    CPU::StateSnapshot initial = myCPU.GetState();
    q.push(initial);
    visited.insert(initial);

    int states_explored = 0;
    
    // Explicit Inputs for Forking
    std::vector<int> test_inputs = {0, 1, 42, 100};

    while (!q.empty()) {
        CPU::StateSnapshot current_snap = q.front();
        q.pop();
        states_explored++;

        myCPU.RestoreState(current_snap);
        unsigned long pc = myCPU.readPC();

        if (pc >= maxPC * 4 || pc >= 4096) continue;

        Instruction myInst(instMem, myCPU);
        unsigned long nextPC = pc + 4;

        Controller myCtrl(myInst);
        ALU_Controller myALU(myInst, myCtrl.ALUOp);
        int32_t ImmVal = ImmGen(myInst);

        int rs1 = (myInst.instr >> 15) & 0x1F;
        int rs2 = (myInst.instr >> 20) & 0x1F;
        int rd = (myInst.instr >> 7) & 0x1F;

        myCPU.registers[0] = 0;
        int rs1Val = myCPU.registers[rs1];
        int rs2Val = myCPU.registers[rs2];
        int rs2Val_mux = myCtrl.AluSrc ? ImmVal : rs2Val;
        
        int32_t ALU_Res = ALU_Result(rs1Val, rs2Val_mux, myALU.ALUOp);

        if (!VerifyState(myCPU, myInst, myCtrl, ALU_Res)) return;

        // NON-DETERMINISM FORK
        bool isInput = (myCtrl.MemRe && ALU_Res == CPU::MMIO_INPUT_ADDR);
        std::vector<int> inputs_to_try;
        
        if (isInput) inputs_to_try = test_inputs;
        else {
            bool isWord = ((myInst.instr & 0x7000) == 0x2000);
            inputs_to_try.push_back(myCPU.DataMemory(myCtrl.MemWr, myCtrl.MemRe, ALU_Res, rs2Val, isWord));
        }

        for (int val : inputs_to_try) {
            CPU forkCPU = myCPU;
            
            if (myCtrl.regWrite && rd != 0) {
                if (myCtrl.opcode == 0x6F) forkCPU.registers[rd] = pc + 4;
                else forkCPU.registers[rd] = myCtrl.MemtoReg ? val : ALU_Res;
            }

            unsigned long forkNextPC = nextPC;
            bool zero = (ALU_Res == 0);
            if (myCtrl.Branch && zero) forkNextPC = pc + ImmVal;
            
            forkCPU.incPC(forkNextPC);

            CPU::StateSnapshot next_state = forkCPU.GetState();
            if (visited.find(next_state) == visited.end()) {
                visited.insert(next_state);
                q.push(next_state);
            } else {
                std::cout << "[FAIL] Loop Detected! Value: " << val << std::endl;
                return;
            }
        }
    }
    std::cout << ">>> VERIFICATION SUCCESSFUL!" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) return -1;
    unsigned char instMem[4096] = {0};
    std::ifstream infile(argv[1]);
    if (!infile.is_open()) return 0;

    std::string line;
    int i = 0;
    while (std::getline(infile, line) && i < 4096) {
        std::stringstream line2(line);
        int hexValue;
        line2 >> std::hex >> hexValue;
        instMem[i] = static_cast<char>(hexValue);
        i++;
    }
    RunBFS(instMem, i / 4);
    return 0;
}
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <cassert>
#include <ctime>
#include <string>
#include <sstream>
#include "CPU_Files/CPU.h" 

using namespace std;

// --- STAGE 1: Random Fuzzing ---
vector<unsigned char> generateRandomInstruction() {
    vector<unsigned char> inst(4);
    for (int i = 0; i < 4; ++i) inst[i] = rand() % 256;
    return inst;
}

// --- STAGE 2: Opcode-Aware Fuzzing (Smart Fuzzing) ---
// Generates instructions that actually look like RISC-V (R-type, I-type, etc.)
vector<unsigned char> generateOpcodeInstruction() {
    // List of common valid opcodes (R-type, I-type, Store, Branch, etc.)
    vector<int> opcodes = {0x33, 0x13, 0x23, 0x6F, 0x37, 0x03}; 
    int opcode = opcodes[rand() % opcodes.size()];
    
    // Construct a semi-valid 32-bit instruction
    uint32_t instr = 0;
    instr |= opcode; // Set opcode (bits 0-6)
    instr |= (rand() % 32) << 7;  // Random rd
    instr |= (rand() % 8) << 12;  // Random func3
    instr |= (rand() % 32) << 15; // Random rs1
    instr |= (rand() % 32) << 20; // Random rs2 or imm
    instr |= (rand() % 128) << 25;// Random func7 or imm

    // Split back into bytes (Little Endian for RISC-V)
    vector<unsigned char> bytes(4);
    bytes[0] = instr & 0xFF;
    bytes[1] = (instr >> 8) & 0xFF;
    bytes[2] = (instr >> 16) & 0xFF;
    bytes[3] = (instr >> 24) & 0xFF;
    return bytes;
}

// --- Helper: Read Hex from File (For AI Integration) ---
vector<unsigned char> loadInstructionsFromFile(const string &filename) {
    vector<unsigned char> instructions;
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
        // Assume file has one hex byte per line or generic hex string
        try {
            instructions.push_back((unsigned char)stoi(line, nullptr, 16));
        } catch (...) { continue; } // Skip bad lines
    }
    return instructions;
}

// --- Helper: Save Instructions to File ---
void saveInstructions(const vector<unsigned char> &instructions, const string &filename) {
    ofstream out(filename);
    if (!out) {
        cerr << "Error: Could not open file " << filename << " for writing." << endl;
        return;
    }
    for (auto byte : instructions) {
        // Write each byte as a 2-digit hex value
        out << hex << (int)byte << endl;
    }
    out.close();
}

// --- STAGE 5: The Judge (Oracle) ---
// Returns true if the state is valid, false otherwise.
bool judgeState(int registers[], int pc) {
    // 1. Invariant: x0 must ALWAYS be 0
    if (registers[0] != 0) {
        cerr << "[JUDGE FAIL] Register x0 is corrupted! Value: " << registers[0] << endl;
        return false;
    }
    // 2. PC Alignment: PC must be divisible by 4
    if (pc % 4 != 0) {
        cerr << "[JUDGE FAIL] PC Misaligned: " << pc << endl;
        return false;
    }
    return true;
}

void runCPU(vector<unsigned char> &instructions) {
    CPU myCPU;
    const int NUM_REGISTERS = 32;
    int registers[NUM_REGISTERS] = {0};

    // Initialize Judge
    bool passed = true;

    size_t maxPC = instructions.size();
    
    // SAFETY: Bounds check before execution loop
    while (myCPU.readPC() + 4 <= maxPC) {
        // ... (Your existing decoding logic remains here) ...
        Instruction myInst(instructions.data(), myCPU);
        Controller myController(myInst);
        ALU_Controller myALU_Control(myInst, myController.ALUOp);
        int32_t ImmValue = ImmGen(myInst);
        
        // ... (Decode rd, rs1, rs2) ...
        int rd = (myInst.instr[11] << 4) | (myInst.instr[10] << 3) | (myInst.instr[9] << 2) | (myInst.instr[8] << 1) | (myInst.instr[7]);
        int rs1 = (myInst.instr[19] << 4) | (myInst.instr[18] << 3) | (myInst.instr[17] << 2) | (myInst.instr[16] << 1) | (myInst.instr[15]);
        int rs2 = (myInst.instr[24] << 4) | (myInst.instr[23] << 3) | (myInst.instr[22] << 2) | (myInst.instr[21] << 1) | (myInst.instr[20]);

        // Execute
        registers[0] = 0; // Hardware tie-down
        int rs1Val = registers[rs1];
        int rs2Val = myController.AluSrc ? ImmValue : registers[rs2];
        int32_t ALU_Res = ALU_Result(rs1Val, rs2Val, myALU_Control.ALUOp);
        bool zeroFlag = ALU_Res ? 0 : 1;

        unsigned long nextPC = myCPU.readPC() + 4;
        if ((myController.Branch == zeroFlag) && (zeroFlag == 1)) {
            nextPC = myCPU.readPC() + ImmValue;
        }

        // Memory Logic (Assuming your CPU class handles bounds, otherwise ASan catches this)
        int32_t Read_Data = myCPU.DataMemory(myController.MemWr, myController.MemRe, ALU_Res, registers[rs2], true);

        // --- ADD CRASH HANDLER ---
        if (myCPU.errorFlag) {
            cout << "\n[CRASH DETECTED] " << myCPU.errorMessage << endl;
            cout << "Faulting Instruction Hex: " << hex << myInst.instr.to_ulong() << dec << endl;
        
            // Save the trace up to this point
            cout << "Saving crash trace to 'Test/crash_trace.txt'..." << endl;
        
            // Create a sub-vector of instructions up to the current PC + 4
            // (Assuming you want the history of what led to the crash)
            size_t currentByteSize = myCPU.readPC() + 4;
            if (currentByteSize > instructions.size()) currentByteSize = instructions.size();
        
            vector<unsigned char> crashTrace(instructions.begin(), instructions.begin() + currentByteSize);
            saveInstructions(crashTrace, "Test/crash_trace.txt");
        
            return; // Exit the function gracefully
        }

        if (myController.regWrite && rd != 0) { // Don't write to x0
            registers[rd] = myController.MemtoReg ? Read_Data : ALU_Res;
        }

        // --- INVOKE JUDGE ---
        if (!judgeState(registers, nextPC)) {
            passed = false;
            break; 
        }

        myCPU.incPC(nextPC);
    }

    if (passed) cout << "[JUDGE] Execution Valid." << endl;
    else cout << "[JUDGE] Execution FAILED constraints." << endl;
}

int main(int argc, char* argv[]) {
    srand(time(0));
    vector<unsigned char> instructions;
    string mode = (argc > 1) ? argv[1] : "random";

    if (mode == "random") {
        cout << "Running Random Fuzzer..." << endl;
        for (int i = 0; i < 1000000; ++i) {
            auto inst = generateRandomInstruction();
            instructions.insert(instructions.end(), inst.begin(), inst.end());
        }
        // --- DYNAMIC FILENAME GENERATION ---
        string outputFilename = mode + "_debug_instructions.txt";
        cout << "Saving generated instructions to '" << outputFilename << "'..." << endl;
        saveInstructions(instructions, outputFilename);
        // -----------------------------------
    } else if (mode == "opcode") {
        cout << "Running Opcode-Aware Fuzzer..." << endl;
        for (int i = 0; i < 100000; ++i) {
            auto inst = generateOpcodeInstruction();
            instructions.insert(instructions.end(), inst.begin(), inst.end());
        }
        // --- DYNAMIC FILENAME GENERATION ---
        string outputFilename = mode + "_debug_instructions.txt";
        cout << "Saving generated instructions to '" << outputFilename << "'..." << endl;
        saveInstructions(instructions, outputFilename);
    } else if (mode == "file") {
        cout << "Running File-Input Fuzzer (AI Trace)..." << endl;
        if (argc < 3) { cerr << "Provide filename"; return 1; }
        instructions = loadInstructionsFromFile(argv[2]);
    }



    runCPU(instructions);
    return 0;
}
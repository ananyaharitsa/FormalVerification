# FormalVerification

#  RISC-V CPU Simulator — Code Overview

This program implements a simplified **RISC-V CPU simulator** in C++.
It models the core stages of instruction execution: **fetch, decode, execute, memory access, and write-back**.

# Single-Cycle RISC-V CPU Simulator & Verification Framework

This repository contains a **Single-Cycle RISC-V CPU Simulator** written in **C++**, along with a comprehensive suite of **verification and analysis tools**, including:

* **Static Analysis** (Cppcheck, Clang-Tidy)
* **Dynamic Fuzzing** (Random, Opcode-Aware, GenAI)
* **Explicit State Model Checking** (BFS-based)
* **Symbolic Execution / Bounded Model Checking** (CBMC)

The goal of this project is not only functional correctness, but *high assurance* through multiple complementary verification techniques.

---

## 📂 Project Structure

```
.
├── CPU_Files/                 # Core RISC-V simulator implementation
│   ├── cpusim.cpp
│   ├── CPU.cpp
│   └── CPU.h
│
├── DynamicAnalysis/           # Dynamic fuzzing pipeline
│   ├── fuzzer.cpp
│   ├── ai_gemini_fuzzer.py
│   ├── run_pipeline.sh
│   └── ...
│
├── ExplicitModelChecking/     # BFS-based explicit state model checker
│   ├── ModelChecker.cpp
│   └── CPU.cpp
│
├── CBMC/                      # CBMC symbolic execution harnesses
│   └── CPU_Files/
│       ├── cpu_cbmc_harness.cpp
│       └── CPU.cpp
│
├── NondeterministicCBMC/      # Nondeterministic CBMC experiments
│
├── static_analysis_reports/   # Cppcheck and Clang-Tidy output logs
│
├── Test/                      # Instruction traces and hex dumps
│   └── trace/
│       └── 24swr.txt
│
└── README.md
```

---

## 🛠️ Prerequisites

To run **all components** of this project, you will need:

* **C++ Compiler**: `g++` (C++17 support required)
* **Python 3**: For GenAI fuzzing scripts
* **Valgrind**: Memory leak detection (Linux / WSL)
* **CBMC**: C Bounded Model Checker (formal verification)
* **Sanitizers**: GCC AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan)

> **Windows users**: Use **WSL** or **MSYS2** to access `g++`, `valgrind`, and `cbmc`.
> 
To debug the CPU simulator in VS Code, first download MSYS2 from [https://www.msys2.org](https://www.msys2.org) and install it in `C:\msys64` (use `MSYS2` as the Start Menu folder). Open the **MSYS2 MSYS** terminal and update packages with `pacman -Syu` (run twice if prompted). Then install GCC and GDB using `pacman -S mingw-w64-x86_64-gcc` and `pacman -S mingw-w64-x86_64-gdb`. The debugger will be at `C:\msys64\mingw64\bin\gdb.exe`; this path in the VS Code `launch.json` under `"miDebuggerPath"` already.


---

## 1. 🖥️ Running the CPU Simulator (Standard Execution)

The core simulator reads a file of hexadecimal RISC-V machine code and executes it instruction by instruction.

### 🔨 Build

From the repository root:

```bash
g++ -std=c++17 -o cpusim.exe CPU_Files/cpusim.cpp CPU_Files/CPU.cpp -I CPU_Files
```

### ▶️ Run

Provide a trace file containing hexadecimal instructions:

```bash
./cpusim.exe Test\trace\24instMem-swr.txt
```

Replace `Test/trace/24instMem-swr.txt` with any valid instruction trace. The readout for the actual instruction is in the same folder for reference at `Test/trace/24swr.txt`

When you run the program with this file as an argument, the CPU simulator will execute all instructions in the file and display the results in the terminal. The output includes the final contents of the registers, for example, `(a0, a1)`, showing the state of the CPU at the end of execution.

---

## 2. 💥 Running Dynamic Analysis (Fuzzing Pipeline)

This project includes an **automated fuzzing pipeline** that performs:

1. Random Fuzzing
2. Opcode-Aware Fuzzing
3. GenAI-Based Fuzzing

All fuzzers are executed under **ASan/UBSan** and followed by **Valgrind** analysis.

### Option A: Automated Pipeline (Recommended)

This script:

* Generates AI-based instruction traces
* Compiles fuzzers with sanitizers
* Runs all fuzzing stages
* Executes Valgrind for leak detection

```bash
cd DynamicAnalysis
chmod +x run_pipeline.sh
./run_pipeline.sh
```

---

### Option B: Manual Execution

#### 1️⃣ Generate GenAI Traces (API key/environment setup required)

```bash
python3 ai_gemini_fuzzer.py gen
python3 ai_gemini_fuzzer.py edge
```

#### 2️⃣ Compile with Address & Undefined Behavior Sanitizers

```bash
g++ -std=c++17 -fsanitize=address,undefined -g \
    -o fuzzer_asan fuzzer.cpp CPU_Files/CPU.cpp -I CPU_Files
```

#### 3️⃣ Run Individual Fuzzer Modes

```bash
./fuzzer_asan random                    # Stage 1: Random fuzzing
./fuzzer_asan opcode                    # Stage 2: Opcode-aware fuzzing
./fuzzer_asan file ai_trace_gen.txt     # Stage 3: GenAI fuzzing
```

---

## 3. 📐 Running Formal Verification

### A. Explicit State Model Checker (BFS)

This tool performs a **Breadth-First Search** over the CPU state space to detect:

* Infinite loops
* Illegal or unsafe states

#### Build

```bash
cd ExplicitModelChecking
g++ -std=c++17 -o modelchecker ModelChecker.cpp CPU.cpp -I .
```

#### Run

```bash
./modelchecker program.txt
```

---

### B. Symbolic Execution with CBMC

CBMC is used to *mathematically prove* properties such as:

* No buffer overflows
* No signed integer overflows
* Assertion correctness

#### Run CBMC

```bash
cd CBMC/CPU_Files

# --unwind 10: Unroll loops 10 times
# --trace: Display counterexamples if a property fails
cbmc cpu_cbmc_harness.cpp CPU.cpp --unwind 10 --trace
```

---

## 4. 🔍 Static Analysis

Static analysis is performed using:

* **Cppcheck**
* **Clang-Tidy**

Generated reports are stored in:

```
static_analysis_reports/
```

### Run Cppcheck Manually

```bash
cppcheck --enable=all --inconclusive --std=c++17 CPU_Files/
```

---

## 📝 Debugging & Development Notes

* **VS Code**:

  * A `.vscode/tasks.json` file is included.
  * Use: `Terminal → Run Task → Build CPU Simulator` to compile directly inside VS Code.

* **Windows**:

  * Use **WSL** or **MSYS2** for best compatibility.
  * If debugging with GDB, ensure `miDebuggerPath` is correctly set in `launch.json`.

---

## ✅ Summary

This repository demonstrates a **defense-in-depth verification strategy** for a RISC-V CPU implementation by combining:

* Conventional execution testing
* Memory-safe fuzzing
* Explicit state exploration
* Formal symbolic reasoning

Together, these techniques significantly increase confidence in correctness, safety, and robustness.



# CODE BACKGROUND
---
Each class corresponds to a specific component in a basic CPU datapath.
---
##  1. `CPU` Class

### Purpose:

Represents the **central processing unit**, containing:

* **Program Counter (PC)**: keeps track of the current instruction’s memory address.
* **Data Memory (dmemory)**: an array that simulates main memory.

### Key Functions:

* `CPU::CPU()`:
  Initializes the CPU with `PC = 0` and clears data memory (`dmemory`) to zero.
* `readPC()`:
  Returns the current program counter value.
* `incPC(nextPC)`:
  Updates the program counter (used to move to the next instruction).
* `DataMemory(MemWrite, MemRead, ALUResult, rs2, word)`:
  Handles **memory access**:

  * If `MemWrite` = 1, stores data (`rs2`) at the given memory address (`ALUResult`).
  * If `MemRead` = 1, loads data from memory at `ALUResult`.
  * Handles both **word-level** (32-bit) and **byte-level** operations.

---

##  2. `Instruction` Class

### Purpose:

Simulates the **Instruction Fetch** stage.

When an instruction is created, it:

* Reads 4 bytes from instruction memory at the address pointed to by `PC`.
* Combines them into a single 32-bit instruction (`instr`).
* Converts it from **little-endian** to **big-endian** format for proper bit ordering.

### Function:

```cpp
Instruction::Instruction(unsigned char instructionMem[], CPU cpu)
```

* Fetches 4 bytes starting at `cpu.PC`.
* Shifts and combines bytes into a 32-bit `bitset<32>` (`instr`).

Helper function:

```cpp
uint32_t toBigEndian(uint32_t value)
```

Converts a 32-bit integer from little-endian to big-endian order.

---

##  3. `Controller` Class

### Purpose:

Implements the **Main Control Unit** of the CPU.

It decodes the **opcode** from the instruction to determine the instruction type and set control signals.

### Supported RISC-V Instruction Types:

| Type   | Opcode (binary) | Example       |
| ------ | --------------- | ------------- |
| R-Type | `0110011`       | `ADD`, `XOR`  |
| I-Type | `0010011`       | `ORI`, `SRAI` |
| U-Type | `0110111`       | `LUI`         |
| L-Type | `0000011`       | `LW`, `LB`    |
| S-Type | `0100011`       | `SW`, `SB`    |
| B-Type | `1100011`       | `BEQ`         |
| J-Type | `1101111`       | `JAL`         |

### Output Control Signals:

| Signal     | Meaning                                            |
| ---------- | -------------------------------------------------- |
| `regWrite` | Write result to register file                      |
| `AluSrc`   | Choose between register or immediate for ALU input |
| `Branch`   | Indicates a branch instruction                     |
| `MemRe`    | Memory read enable                                 |
| `MemWr`    | Memory write enable                                |
| `MemtoReg` | Select memory or ALU output for write-back         |
| `ALUOp`    | Tells ALU what operation to perform                |

---

## 4. `ALU_Controller` Class

### Purpose:

Implements the **ALU Control Unit**, which uses both the opcode (`ALUOp`) and function bits (`funct3` / `funct7`) to determine the specific ALU operation.

### Example:

* For R-Type (`ALUOp = 10`):

  * `ADD` → `ALUOp = 0010`
  * `XOR` → `ALUOp = 0011`
* For I-Type:

  * `ORI` → `0001`
  * `SRAI` → `0100`
* For U-Type:

  * `LUI` → `1000`
* For Branch (`BEQ`) → `0110`
* For Jump (`JAL`) → `1111`

---

## 5. `ALU_Result()` Function

### Purpose:

Implements the **Arithmetic Logic Unit (ALU)**, which performs the arithmetic or logical operation specified by `ALUOp`.

### Supported Operations:

| ALUOp  | Operation | Description                |
| ------ | --------- | -------------------------- |
| `0010` | ADD       | Adds two operands          |
| `0110` | SUB       | Used for subtraction / BEQ |
| `0000` | AND       | Bitwise AND                |
| `0001` | OR        | Bitwise OR                 |
| `0011` | XOR       | Bitwise XOR                |
| `0100` | SRAI      | Shift Right Arithmetic     |
| `1000` | LUI       | Load upper immediate       |
| `1111` | JAL       | Jump and link (returns 0)  |

---

## 6. `ImmGen()` Function

### Purpose:

Generates the **immediate value** (`ImmGen`) for different instruction types based on how immediate bits are laid out in the instruction format.

### Immediate Extraction Logic:

| Type   | Immediate Bits                    | Description         |
| ------ | --------------------------------- | ------------------- |
| I-Type | bits [31:20]                      | 12-bit immediate    |
| S-Type | bits [31:25] + [11:7]             | Split immediate     |
| B-Type | bits [31], [7], [30:25], [11:8]   | Branch offset       |
| U-Type | bits [31:12]                      | Upper 20 bits (LUI) |
| J-Type | bits [31], [19:12], [20], [30:21] | Jump target offset  |
| R-Type | —                                 | No immediate        |

Each result is **sign-extended** to 32 bits if necessary.

---

## 7. Memory Access: `CPU::DataMemory()`

### Purpose:

Simulates **data memory read/write** operations depending on control signals.

### Behavior:

* **Store Word (SW)** → writes 32-bit data into memory.
* **Store Byte (SB)** → writes only the lower byte.
* **Load Word (LW)** → reads full 32-bit data.
* **Load Byte (LB)** → reads only 1 byte (lower 8 bits).

### Example:

```cpp
int32_t DataMemory(int MemWrite, int MemRead, int ALUResult, int rs2, bool word)
```

* `MemWrite = 1` → write `rs2` into memory.
* `MemRead = 1` → return value stored at memory[ALUResult].
* `word` flag chooses between word (4-byte) and byte (1-byte) operations.

---

## Summary of CPU Flow

| Stage       | Function           | Description                                            |
| ----------- | ------------------ | ------------------------------------------------------ |
| Fetch       | `Instruction()`    | Load 4 bytes into `instr`                              |
| Decode      | `Controller()`     | Determine control signals                              |
| ALU Control | `ALU_Controller()` | Determine ALU operation                                |
| Execute     | `ALU_Result()`     | Perform arithmetic/logical operation                   |
| Memory      | `DataMemory()`     | Read or write data memory                              |
| Writeback   | —                  | Write results back to registers (not implemented here) |

---

## Summary

This C++ program builds the **core logic** of a single-cycle RISC-V CPU simulator, covering:

* Instruction fetching and decoding
* Control signal generation
* ALU control and execution
* Immediate value extraction
* Memory read/write functionality

It provides a solid foundation for simulating and testing RISC-V instructions step-by-step.

```

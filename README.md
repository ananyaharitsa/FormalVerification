# FormalVerification

#  RISC-V CPU Simulator — Code Overview

This program implements a simplified **RISC-V CPU simulator** in C++.
It models the core stages of instruction execution: **fetch, decode, execute, memory access, and write-back**.

# How to Run the CPU Simulator

1. Open the `cpusim.cpp` file in your project folder.
2. Build the program. In VS Code, you can do this by pressing `Ctrl+Shift+B` and selecting the appropriate build task. Make sure all dependent files (`CPU.cpp`, `CPU.h`, and any other headers or source files) are in the same folder or included in your build command.
3. Once the build completes, open a terminal in the same directory where `cpusim.exe` was created.
4. Execute the program by providing your instruction file as a command-line argument. For example:

```powershell
.\cpusim.exe CPU_Files\your_input_file.txt
```

Replace `your_input_file.txt` with the name of the instruction file you want to simulate.  

When you run the program with this file as an argument, the CPU simulator will execute all instructions in the file and display the results in the terminal. The output includes the final contents of the registers, for example `(a0, a1)`, showing the state of the CPU at the end of execution.


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
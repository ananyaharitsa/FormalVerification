#!/bin/bash

# Compile Flags
CXX=g++
FILES="fuzzer.cpp ../CPU_Files/CPU.cpp" # Adjust paths as needed
FLAGS="-std=c++17 -I../CPU_Files"

echo "========================================"
echo "    RISC-V HYBRID FUZZING PIPELINE      "
echo "========================================"

# --- STEP 1: AI GENERATION ---
echo "[+] Step 1: Generating AI Traces..."
python3 ai_gemini_fuzzer.py gen
python3 ai_gemini_fuzzer.py edge

# --- STEP 2: COMPILE WITH SANITIZERS (ASan + UBSan) ---
echo "[+] Step 2: Compiling with AddressSanitizer & UBSan..."
$CXX $FLAGS -fsanitize=address,undefined -g -o fuzzer_asan $FILES

echo "[!] Running Random Fuzzing (Sanitized)..."
./fuzzer_asan random > /dev/null

echo "[!] Running Opcode Fuzzing (Sanitized)..."
./fuzzer_asan opcode > /dev/null

echo "[!] Running AI Gen Fuzzing (Sanitized)..."
./fuzzer_asan file ai_trace_gen.txt > /dev/null

echo "[!] Running AI Edge Fuzzing (Sanitized)..."
./fuzzer_asan file ai_trace_edge.txt > /dev/null

if [ $? -eq 0 ]; then
    echo "    -> Sanitizer Check PASSED (No crashes found)"
else
    echo "    -> Sanitizer Check FAILED (Crash detected!)"
fi

# --- STEP 3: COMPILE NORMAL & RUN VALGRIND ---
echo "[+] Step 3: Compiling for Valgrind (No Sanitizers)..."
$CXX $FLAGS -g -o fuzzer_valgrind $FILES

echo "[!] Running Valgrind on Opcode Fuzzer..."
valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./fuzzer_valgrind opcode > valgrind_log.txt 2>&1

if [ $? -eq 0 ]; then
    echo "    -> Valgrind Check PASSED"
else
    echo "    -> Valgrind Check FAILED. Check valgrind_log.txt"
    cat valgrind_log.txt | grep "Invalid"
fi

echo "========================================"
echo "    PIPELINE COMPLETE                   "
echo "========================================"
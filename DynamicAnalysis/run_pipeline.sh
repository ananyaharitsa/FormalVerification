#!/bin/bash

# Compile Flags
CXX=g++
FILES="fuzzer.cpp CPU_Files/CPU.cpp" 
FLAGS="-std=c++17 -ICPU_Files"

echo "========================================"
echo "    RISC-V HYBRID FUZZING PIPELINE      "
echo "========================================"

mkdir -p Test

# --- STEP 1: AI GENERATION ---
echo "[+] Step 1: Generating AI Traces..."
if [ -f "ai_gemini_fuzzer.py" ]; then
    python3 ai_gemini_fuzzer.py gen
    python3 ai_gemini_fuzzer.py edge
else
    python3 ai_fuzzer.py gen
    python3 ai_fuzzer.py edge
fi

# --- STEP 2: COMPILE WITH SANITIZERS (ASan + UBSan) ---
echo "[+] Step 2: Compiling with AddressSanitizer & UBSan..."
$CXX $FLAGS -fsanitize=address,undefined -g -o fuzzer_asan $FILES

# Create/Clear the log file
rm -f sanitizer_log.txt
touch sanitizer_log.txt

echo "[!] Running Random Fuzzing (Sanitized)..."
# 2>&1 redirects Errors (stderr) to the log file, so you capture the crash details
./fuzzer_asan random >> sanitizer_log.txt 2>&1

echo "[!] Running Opcode Fuzzing (Sanitized)..."
./fuzzer_asan opcode >> sanitizer_log.txt 2>&1

echo "[!] Running AI Gen Fuzzing (Sanitized)..."
./fuzzer_asan file ai_trace_gen.txt >> sanitizer_log.txt 2>&1

echo "[!] Running AI Edge Fuzzing (Sanitized)..."
./fuzzer_asan file ai_trace_edge.txt >> sanitizer_log.txt 2>&1

# Check if the log contains any "ERROR" or "runtime error" messages
if grep -q -E "ERROR:|runtime error:" sanitizer_log.txt; then
    echo "    -> Sanitizer Check FAILED (Crash detected!)"
    echo "    -> Preview of Crash (First 5 lines):"
    grep -E "ERROR:|runtime error:" -A 5 sanitizer_log.txt | head -n 5
else
    echo "    -> Sanitizer Check PASSED (No crashes found)"
fi

# --- STEP 3: COMPILE NORMAL & RUN VALGRIND ---
echo "[+] Step 3: Compiling for Valgrind (No Sanitizers)..."
$CXX $FLAGS -DVALGRIND_MODE -g -o fuzzer_valgrind $FILES

rm -f valgrind_log.txt
touch valgrind_log.txt
FAIL_FLAG=0

echo "[!] Running Valgrind Suite..."

run_valgrind_mode() {
    MODE_NAME=$1
    CMD_ARGS=$2
    echo "    ...Scanning $MODE_NAME"
    
    valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./fuzzer_valgrind $CMD_ARGS >> valgrind_log.txt 2>&1
    
    if [ $? -ne 0 ]; then
        FAIL_FLAG=1
        echo "       [!] Bug detected in $MODE_NAME!"
    fi
}

# 1. VERIFY CRASH TRACE
if [ -f "Test/crash_trace.txt" ]; then
    run_valgrind_mode "Saved Crash Trace" "file Test/crash_trace.txt"
else
    echo "    (No crash trace found, skipping verification)"
fi

# 2. RUN ALL MODES
run_valgrind_mode "Random Fuzzer" "random"
run_valgrind_mode "Opcode Fuzzer" "opcode"
run_valgrind_mode "AI Gen Trace" "file ai_trace_gen.txt"
run_valgrind_mode "AI Edge Trace" "file ai_trace_edge.txt"


if [ $FAIL_FLAG -eq 0 ]; then
    echo "    -> Valgrind Check PASSED (No errors in any mode)"
else
    echo "    -> Valgrind Check FAILED. Check valgrind_log.txt"
    echo "    -> Preview of Errors:"
    grep "Invalid" valgrind_log.txt | head -n 5
fi

echo "========================================"
echo "    PIPELINE COMPLETE                   "
echo "========================================"
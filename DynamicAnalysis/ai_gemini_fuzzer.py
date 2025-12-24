import google.generativeai as genai
import os
import sys
import re
import asyncio
from dotenv import load_dotenv

load_dotenv()
api_key = os.getenv("GOOGLE_API_KEY")

# Configure the Gemini API
genai.configure(api_key=api_key)

# Initialize the model 
model = genai.GenerativeModel('gemini-flash-latest')

# --- STRICTER PROMPTS ---
PROMPT_GEN = (
    "Generate 50 random valid RISC-V 32-bit machine code instructions. "
    "Output ONLY the raw hex bytes. Do not include '0x' prefixes. "
    "Do not include assembly code, comments, or conversational text. "
    "Just a continuous stream of hex characters."
)

PROMPT_EDGE = (
    "Generate a RISC-V machine code sequence that tests edge cases: "
    "assigning zero register, jumping to address 0, and accessing unaligned memory. "
    "Output ONLY the raw hex bytes (machine code). "
    "Do not output the assembly instructions. "
    "Do not output markdown formatting. "
    "Just the pure hex digits."
)

async def get_ai_instructions(prompt, filename):
    print(f"Requesting AI Trace for: {filename}...")
    
    # --- CHANGED: Using Google SDK instead of Poe ---
    try:
        # We use await here to keep your async structure intact
        response = await model.generate_content_async(prompt)
        full_text = response.text
    except Exception as e:
        print(f"Error calling Gemini API: {e}")
        return


    # 1. Smart Parsing (The Filter)
    # First, look for code blocks if they exist (between ```)
    code_blocks = re.findall(r"```(?:plaintext|assembly)?\n(.*?)```", full_text, re.DOTALL)
    
    source_text = full_text
    if code_blocks:
        # If the AI used code blocks, the good stuff is likely inside the LAST one
        source_text = code_blocks[-1]

    # Clean up the text: Remove '0x', remove spaces/newlines
    clean_hex_stream = ""
    
    # Process line by line to filter out "conversational" lines that happen to have numbers
    for line in source_text.splitlines():
        # Remove comments (anything after #)
        line = line.split("#")[0].strip()
        
        # Remove '0x' if present
        line = line.replace("0x", "")
        
        # Remove spaces
        line = line.replace(" ", "")

        # Regex check: Is the remaining string ONLY hex digits?
        if re.fullmatch(r'[0-9a-fA-F]+', line):
            clean_hex_stream += line

    # 2. Format into "One Byte Per Line" for the C++ loader
    formatted_output = ""
    for i in range(0, len(clean_hex_stream), 2):
        byte = clean_hex_stream[i:i+2]
        if len(byte) == 2:
            formatted_output += byte + "\n"

    # 3. Save to file
    if len(formatted_output) > 0:
        with open(filename, "w") as f:
            f.write(formatted_output)
        print(f"Saved cleaned trace to {filename} ({len(formatted_output)//3} bytes)")
    else:
        print(f"[!] Warning: No valid hex found in AI response for {filename}")
        print("Raw response was:", full_text) # Helpful for debugging

if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "gen"
    
    if mode == "gen":
        asyncio.run(get_ai_instructions(PROMPT_GEN, "ai_trace_gen.txt"))
    elif mode == "edge":
        asyncio.run(get_ai_instructions(PROMPT_EDGE, "ai_trace_edge.txt"))
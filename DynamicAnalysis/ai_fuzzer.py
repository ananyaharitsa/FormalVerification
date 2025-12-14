import fastapi_poe as fp
import os
import sys
from dotenv import load_dotenv
import asyncio

load_dotenv()
api_key = os.getenv("POE_API_KEY")

# Prompts
PROMPT_GEN = "Generate 50 random valid RISC-V 32-bit machine code instructions. Output ONLY the hex bytes, one byte per line. Do not include comments."
PROMPT_EDGE = "Generate a RISC-V machine code sequence that tests edge cases: division by zero, jumping to address 0, and accessing unaligned memory. Output ONLY the hex bytes, one byte per line."

async def get_ai_instructions(prompt, filename):
    message = fp.ProtocolMessage(role="user", content=prompt)
    print(f"Requesting AI Trace for: {filename}...")
    
    with open(filename, "w") as f:
        async for partial in fp.get_bot_response(
            messages=[message],
            bot_name="GPT-3.5-Turbo", # Or your specific bot
            api_key=api_key
        ):
            if partial.text:
                # Clean output to ensure only hex remains
                clean_text = partial.text.replace("0x", "").replace(" ", "\n")
                f.write(clean_text)
    print(f"Saved trace to {filename}")

if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "gen"
    
    if mode == "gen":
        asyncio.run(get_ai_instructions(PROMPT_GEN, "ai_trace_gen.txt"))
    elif mode == "edge":
        asyncio.run(get_ai_instructions(PROMPT_EDGE, "ai_trace_edge.txt"))
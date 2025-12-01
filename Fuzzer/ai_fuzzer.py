import fastapi_poe as fp
import os
from dotenv import load_dotenv

load_dotenv(dotenv_path=".env")  # reads .env file

message = fp.ProtocolMessage(role="user", content="Hello world")

api_key = os.getenv("POE_API_KEY")
print("API KEY:", api_key)

for partial in fp.get_bot_response_sync(
    messages = [message],
    bot_name = "riscv-instr-bot",
    api_key = api_key
):
    
    print(partial)
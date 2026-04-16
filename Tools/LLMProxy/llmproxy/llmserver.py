# File: ollama_bridge/bridge.py
import argparse
import time
import requests
import subprocess
import signal
import sys
import socket
from fastapi import FastAPI, HTTPException
from fastapi.responses import JSONResponse
from pydantic import BaseModel
import uvicorn
import re
import json

app = FastAPI()
OLLAMA_PORT = 11434

# Global variable to hold the current Ollama process
ollama_proc = None

query_msg_no = 0
last_bench = "None"

###########################################################
#
# Process on each benchmark for extracting result
#
###########################################################
base64_regex = r'base64[\s\S]*?```[^\n]*\n([A-Za-z0-9+/=]+)\s*```'

bench_regexs = {
    "libxml2": [
        r'```[^\n]*\n(.*?)\n```',
        r'<.*>'
    ],

    "arrow": [
        base64_regex,
        r'```[^\n]*\n(.*?)\n```'
    ],

    "avconv": [
        base64_regex,
        r'```[^\n]*\n(.*?)\n```'
    ],

    "uriparser": [
        base64_regex,
        r'```[^\n]*\n(.*?)\n```'
    ],

    "xpdf": [
        base64_regex,
        r'```[^\n]*\n(.*?)\n```'
    ],

    "sablotron": [
        r'```[^\n]*\n(.*?)\n```',
        r'<.*>'
    ],

    "mp3gain ":[
        # r'/(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?/g'
        base64_regex,
        r'```[^\n]*\n(.*?)\n```'
    ],
    
    "exiv2": [
        base64_regex,
        r'```[^\n]*\n(.*?)\n```'
    ],
    "libpng": [
        base64_regex,
        r'```[^\n]*\n(.*?)\n```'
    ],
    "mp4v2": [
        base64_regex,
        r'```[^\n]*\n(.*?)\n```'
    ],
    "mdp": [  
        r'```markdown([\s\S]*?)```',
        r'```md([\s\S]*?)```',
        r'md[\s\S]*:\s*"(.*?)"',
        r'"result"\s*:\s*"(.*?)"',
        r'```[^\n]*\n(.*?)\n```'
    ],
    "pycdc": [
        base64_regex,
        r'```[^\n]*\n(.*?)\n```'
    ],
    "tcpdump": [
        base64_regex,
        r'```[^\n]*\n(.*?)\n```'
    ],
    "tippecanoe": [   
        r'```json([\s\S]*?)```',
        r'"result"\s*:\s*"(.*?)"',
        r'```[^\n]*\n(.*?)\n```'
    ], 
}

bench_regexs_indicator = {
    "mdp": [True, True, True, True, True],
    "tippecanoe": [True, True, True]
}
###########################################################


class ChatCompletionRequest(BaseModel):
    model: str
    messages: list
    max_tokens: int = 256
    temperature: float = 0.9

def strip_think_tags(text: str) -> str:
    """
    Removes all occurrences of <think> ... </think> (including nested content) from a string.
    """
    # DOTALL (the re.S flag) makes '.' match newlines, so multiline <think> blocks are removed too.
    return re.sub(r'<think>.*?</think>', '', text, flags=re.DOTALL)

def is_json_with_result(text: str) -> bool:
    try:
        data = json.loads(text)
        return isinstance(data, dict) and "result" in data
    except json.JSONDecodeError:
        return False

def extract_code_content(benchmark_name: str, text: str) -> str:
    """
    Applies a sequence of regex patterns associated with the given benchmark name to the input text.
    If a regex pattern matches, its capture group (or the entire match if no capture group exists)
    becomes the input for the next regex in the sequence.
    
    Args:
        benchmark_name (str): The benchmark name used to look up the regex list.
        text (str): The input text to be processed.
    
    Returns:
        str: A JSON string with the key "result" containing the final extracted content.
             If the benchmark is not found, the original text is returned in JSON format.
    """
    regex_list = bench_regexs.get(benchmark_name)
    if regex_list is None:
        regex_list = [r'```[^\n]*\n(.*?)\n```'] # default

    indicators = bench_regexs_indicator.get(benchmark_name)
    
    index=0
    cur_text = text
    for pattern in regex_list:
        print(f'@@@@@@@@@@:{index}---{cur_text}')
        
        if cur_text==None:
            cur_text=text
            
        match = re.search(pattern, cur_text, flags=re.DOTALL)
        if match:
            cur_text = match.group(1) if match.lastindex and match.lastindex >= 1 else match.group(0)
            if indicators != None and indicators[index] == True:
                break
        index+=1

    if is_json_with_result(cur_text):
        return cur_text
    
    return json.dumps({"result": cur_text})

def get_benchmark_name(text: str) -> str:
    pattern = r'Project information:\s*([^@]+)@'
    match = re.search(pattern, text)
    return match.group(1) if match else ""

def build_ollama_payload(req: ChatCompletionRequest) -> dict:
    """
    Build the JSON payload expected by Ollama's /api/generate endpoint 
    from a ChatCompletionRequest object.
    """
    global last_bench
    global query_msg_no
    benchmark_name = ""
    user_prompt = ""
    for message in req.messages:
        role = message["role"]
        content = message["content"]
        if len (content) == 0:
            continue

        if role == "system":
            user_prompt += f"System: {content}\n\n"
        elif role == "assistant":
            user_prompt += f"Assistant: {content}\n\n"
        else:
            user_prompt += f"User: {content}\n\n"

    benchmark_name = get_benchmark_name (user_prompt)
    if benchmark_name != last_bench:
        last_bench = benchmark_name
        query_msg_no = 0

    mes_len = len (user_prompt)
    return {
        "model": req.model,
        "prompt": user_prompt.strip(),
        "temperature": req.temperature,
        "num_ctx": 2048,
        "max_tokens": req.max_tokens,
        "stream": False
    }, mes_len, benchmark_name

@app.post("/chat/completions")
def create_chat_completion(req: ChatCompletionRequest):
    global query_msg_no

    ollama_payload, mes_len, benchmark_name = build_ollama_payload(req)
    print(f">>[{benchmark_name}][{query_msg_no}]Inquery@{req.model} with msg length {mes_len}")
    query_msg_no += 1

    try_times = 0
    cleaned_response = ""
    while True:
        try:
            resp = requests.post(
                f"http://localhost:{OLLAMA_PORT}/api/generate",
                json=ollama_payload,
                timeout=60
            )
            print (resp)
            if resp.status_code != 200:
                print (f"@@@create_chat_completion: unexpected return code -> {resp}...")
                reset_model_state()
                continue
            data = resp.json()
        except requests.exceptions.RequestException as e:
            print (f"@@@[{try_times}]create_chat_completion: RequestException {e}...")
            try_times += 1
            if try_times > 1:
                break
            reset_model_state()
            continue

        if "response" not in data:
            print ("@@@create_chat_completion: response not in data...")
            reset_model_state()
            continue

        cleaned_response = strip_think_tags(data["response"])
        if cleaned_response.strip() == "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG":
            print (f"@@@create_chat_completion: model response is abnormal --> {cleaned_response}")
            reset_model_state()
            continue
        break

    print (f"\n>>>>PRE-extract_code_content---> {cleaned_response}")
    cleaned_response = extract_code_content (benchmark_name, cleaned_response)
    print (f"\n>>>>POST-extract_code_content---> {cleaned_response}")
    data["response"] = cleaned_response

    created_ts = int(time.time())
    return JSONResponse({
        "id": f"chatcmpl-{created_ts}",
        "object": "chat.completion",
        "created": created_ts,
        "model": req.model,
        "choices": [{
            "index": 0,
            "message": {"role": "assistant", "content": data["response"]},
            "finish_reason": "stop"
        }],
        "usage": {"prompt_tokens": 0, "completion_tokens": 0, "total_tokens": 0}
    })

def start_ollama_subprocess():
    cmd = ["ollama", "serve"]
    print(f"Starting Ollama: {' '.join(cmd)}")
    ollamaProc = subprocess.Popen(cmd, stdout=None, stderr=None)
    print (ollamaProc)
    return ollamaProc

def shutdown_ollama(proc):
    print("Shutting down Ollama...")
    proc.send_signal(signal.SIGTERM)
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()

def wait_for_port(port, host="127.0.0.1", timeout=30):
    """Wait until a given port is open, or timeout after a given number of seconds."""
    start = time.time()
    while time.time() - start < timeout:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            result = sock.connect_ex((host, port))
            if result == 0:
                return True
        time.sleep(1)
    return False

def reset_model_state():
    """
    Resets the model state by shutting down the current Ollama process
    and starting a new one.
    """
    global ollama_proc
    print("Resetting model state...")
    if ollama_proc is not None:
        shutdown_ollama(ollama_proc)
    ollama_proc = start_ollama_subprocess()
    if not wait_for_port(OLLAMA_PORT, "127.0.0.1", timeout=30):
        print(f"Error: Port {OLLAMA_PORT} did not become available after resetting model state.")
    else:
        print("Model state reset successfully.")
        time.sleep(1)

def main():
    global ollama_proc

    parser = argparse.ArgumentParser(description="Ollama Bridge Server")
    parser.add_argument("--port", type=int, default=5001, help="Port for the proxy server.")
    args = parser.parse_args()

    print(f"Ollama will run on port: {OLLAMA_PORT}")
    ollama_proc = start_ollama_subprocess()
    if not wait_for_port(OLLAMA_PORT, "127.0.0.1", timeout=30):
        print(f"Error: Port {OLLAMA_PORT} did not become available within the timeout period.")
        sys.exit(1)

    pxyPort = 5001
    if args.port:
        pxyPort = args.port

    try:
        uvicorn.run(app, host="0.0.0.0", port=pxyPort)
    except KeyboardInterrupt:
        pass
    finally:
        shutdown_ollama(ollama_proc)
        sys.exit(0)


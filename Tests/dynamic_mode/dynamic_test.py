import os
import sys
import time
import argparse
import json
import yaml
import re
import requests
from langchain_openai import ChatOpenAI
from langchain_core.prompts import PromptTemplate
from pydantic import BaseModel, Field
from langchain_core.output_parsers import JsonOutputParser
from typing import List, Tuple
from langchain_google_genai import GoogleGenerativeAI
#os.environ["GOOGLE_API_KEY"] = "AIzaSyAVj5UemRqtzk7rqCvngH1wQLTtMlE2XWA" #google account: yhryyq
#os.environ["GOOGLE_API_KEY"] = "AIzaSyB4zBw521A70zOYwgPfiMduhpI5cddob8k"
#llm = GoogleGenerativeAI(model="gemini-pro", google_api_key=os.environ["GOOGLE_API_KEY"])
llm = ChatOpenAI(
                model='deepseek-chat',
                openai_api_key="sk-7a43756ae3c847f4b3f45f7b0a98df4f",
                openai_api_base='https://api.deepseek.com',
                max_tokens=8192,
                temperature=0
                )


# Prompt
template = "System: {system_instruction}\n\nQuestion: {question}\nOutput format: {format_instructions}\n"
#template = "Question: {question}\nOutput format: {format_instructions}\n"
#template = "Question: {question}\n"

class str_result(BaseModel):
    result: str = Field(description="Result for str result")

class binary_result(BaseModel):
    binary: str = Field(description="Result for str result")
    forma: str = Field(description="File format")

def query(question, format=str_result, system_prompt=""):
    max_retries = 3
    try_count = 0
    while try_count < max_retries:
        try:
            parser = JsonOutputParser(pydantic_object=format)
            prompt = PromptTemplate(
                template=template,
                input_variables=["question"],
                partial_variables={
                    "system_instruction": system_prompt,
                    "format_instructions": parser.get_format_instructions()
                }
            )
            chain = prompt | llm | parser
            #chain = prompt | llm

            #result = chain.invoke({"question": question})
            result = chain.invoke({"question": question})
            return result
        except Exception as e:
            print(f"OpenAI API Error: {e}")
            try_count += 1
            if try_count == max_retries:
                return {"result": ""}
                #return ""
            time.sleep(10)
            #return {"result": f"Gemini API Error: {e}"}


def GenFuncGroup (directory):
    for filename in os.listdir(directory)[0:1]:
        path = os.path.join(directory, filename)
        try:
            with open(path, 'r', encoding='utf-8') as file:
                content = file.read()
        except e:
            print(f"load the path file error:{e}\n")
            continue
        
        #load yaml info
        try:
            with open('../../Benchmarks/libpng/project.yaml') as f:
                yaml_data = yaml.safe_load(f)
        except e:
            print("load yaml file error")
            yaml_data = ""

        prompt = f"""
                Call path: \n{content}\n\n
                Goal: Generate the input to the project to exercise all functions in the call path above. Save the input to a file in the correct format. The output format must match the format defined in the "input_format" field. Output only the binary stream of the file (with no extra transformations).  """
        question = str(yaml_data) + prompt
        question = """
        Generate a binary representation of a PDF file that contains the text 'hello world'. The PDF file should meet the following requirements:\n
\n
1. The text 'hello world' should be centered on the page, using a 12-point font.\n
2. The PDF structure must comply with the PDF standard, including necessary headers, objects, cross-reference tables, and trailers.\n
\n
Output only the raw binary data of the PDF file, without any additional formatting or explanations.
        """
        #question = str(yaml_data).rstrip('}') + 
        question = f""" "Project": "libhtp",\n
  "Call path:{content},"\n
  "Task": "Generate a raw binary representation of a minimal PCAP file that meets the following requirements:\n\n1. **PCAP File Header** (24 bytes):\n   - Magic Number: 0xA1B2C3D4\n   - Version Major: 2\n   - Version Minor: 4\n   - Link Type: 1 (Ethernet)\n\n2. **Single Packet Header** (16 bytes):\n   - Timestamp (seconds and microseconds)\n   - Packet Length (including Ethernet header)\n   - Captured Length\n\n3. **Packet Data**:\n   - Contains a simple HTTP request or response\n   - Ensure the packet length matches the length specified in the header\n\n4. **The binary data** should only include the above content, with no additional formatting, explanations, or metadata.\n\n5. **Ensure the PCAP file** can be used as a test input for the libhtp project and covers the above call path",
  "Output": "Provide only the  raw binary data of PCAP file, without any additional explanations. Ensure the output is concise and avoids unnecessary padding or redundant data."}}
        """
      
        systemp = f"You are a tester working on testing a project, this project is:{str(yaml_data)}. You need to generate test inputs with composite requirements to test this project."
        question = f"""
        Task:\n
  Generate a minimal valid PCAP file with exactly one packet.\n
  It must:\n
    1. Contain a correct global header (24 bytes) matching the PCAP standard:\n
       - Magic Number: 0xA1B2C3D4\n
       - Version (Major=2, Minor=4)\n
       - Timezone/Accuracy (typically zero)\n
       - Snap Length (max capture length)\n
       - Link Type (e.g., 1 for Ethernet)\n
    2. Contain one valid packet header (16 bytes), with proper timestamps (can be zero) \n
       and the exact captured/original length matching the packet payload size.\n
    3. The single Ethernet frame should carry a minimal HTTP request \n
       (e.g. "GET / HTTP/1.0\r\n\r\n") so that it can serve as a valid test input to libhtp.\n
    4. The data should be specifically crafted to exercise the following call path:\n
       {content}\n
    5. Output must be **only** the Base64 encoding of the PCAP file
       without any extra commentary, JSON, or formatting.\n 
    6. The data should be truly minimal—avoid unnecessary padding or redundant 0x00 bytes.
       The total length should be just enough for a valid PCAP header + one minimal packet header + that packet's data.\n

Output:
  Please provide only the PCAP file data in Base64 encoding, nothing else.Unless necessary content needs to be output, the output length must not exceed 100 chars! NO unnecessary padding and redundant data!
        """
        
        question = f"""
        Task:\n
  Generate a minimal valid Png file.\n
  It must:\n
    1. Contain the fixed 8-byte file signature..\n
    2. Contain IHDR Chunk (Image Header).\n
    3. Contain IDAT Chunk(s) (Image Data).\n
    4. Contain IEND chunk marking the end of the file.
    5. The data should be specifically crafted to exercise the following call path:\n
       {content}\n
    6. Output must be **only** the Base64 encoding of the PNG file
       without any extra commentary, JSON, or formatting.\n
    7. The data should be truly minimal—avoid unnecessary padding or redundant 0x00 bytes.
       The total length should be just enough for a PNG file structure.\n

Output:
  Please provide only the PNG file data in Base64 encoding, nothing else.Unless necessary content needs to be output, the output length must not exceed 100 chars! NO unnecessary padding and redundant data!
        """

        res = query(question, system_prompt=systemp)
        print(f"----------------------")
        print(f"the json file content: \n{content}\n")
        print(f"----------------------")
        print(f"the result: \n{res}\n")
        print(f"----------------------")
        save_file(res['result'],'./')
        time.sleep(1)


def download_file_from_string(input_string, output_dir="./"):
    #get the URL from str
    url_pattern = r'(https?://[^\s]+)'
    urls = re.findall(url_pattern, input_string)

    if not urls:
        print("URL str format error")
        return

    #check output dir and create
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    for url in urls:
        try:
            # send GET request
            response = requests.get(url, stream=True)
            response.raise_for_status()  #check for request error
            
            # get file name from URL
            file_name = url.split("/")[-1]
            file_path = os.path.join(output_dir, file_name)

            # write file to local
            with open(file_path, 'wb') as file:
                for chunk in response.iter_content(chunk_size=8192):
                    file.write(chunk)

            print(f"Dowload success: {file_path}")

        except requests.exceptions.RequestException as e:
            print(f"Dowload error: {url}, detail: {e}")

def save_file(binary_stream,  outdir):
    binary_stream = binary_stream.encode('utf-8')
    
    os.makedirs(outdir, exist_ok=True)

    file_name = f"output"
    file_path = os.path.join(outdir, file_name)

    with open(file_path, "wb") as file:
        file.write(binary_stream)

    print(f"File saved to {file_path}")



GenFuncGroup('./jsons')

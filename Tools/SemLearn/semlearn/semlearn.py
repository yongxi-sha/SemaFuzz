import os
import psutil
import semgdump
import semcore
import time
import base64
import binascii
from datetime import datetime
from transformers import GPT2Tokenizer

start_time = time.time()

def getMemoryUsage():
    process = psutil.Process(os.getpid())
    memory = process.memory_info().rss / (1024 * 1024)
    return memory

def getTimeCost () -> str:
    totalSec = time.time() - start_time
    hours, remainder = divmod(totalSec, 3600)
    minutes, seconds = divmod(remainder, 60)
    return f"{int(hours)} hours, {int(minutes)} minutes, {seconds:.2f} seconds"

def saveBinary (testName, resData):
    print (f"@@saveBinary: --> {testName}")
    try:
        binData = resData.encode('utf-8')
        binData = base64.b64decode(binData)
    except binascii.Error as e:
        print (f"@@saveBinary: not base64 encoding --> {resData}")
        semcore.saveSemTest(testName, resData) 
        return 0

    return semcore.saveBinaryTest(testName, binData)  

# thread for semantic learning without paths
def semLearnNopaths (pro, llmMngt):
    isBase64   = pro.isEncodingBase64 ()
    proProfile = pro.getStcProfile()
    model = llmMngt.getActiveModel ()
    resHash = 0
    resLen  =0 
    while True:
        system_prompt = f"You are a tester for {str(proProfile)}. You need to randomly generate **diverse** test inputs with composite requirements to test this project."
        question = f"Ensure being different from last generation (djb2-hash={resHash}, len={resLen})"

        timeCost = getTimeCost ()
        print (f">>>[{timeCost}] query -->[{question}]")
        res = model.query(question, system_prompt = system_prompt) 
        if res == None:
            continue
        if 'result' in res:
            resData = str(res['result'])
            resLen  = len (resData)
            testName = datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
            if not isBase64:
                resHash = semcore.saveSemTest(os.path.basename(testName), resData)
            else:
                resHash = saveBinary (testName, resData)
            #print(f"----------------------")
            #print(f"the result: \n{res}\n")
            #print(f"----------------------")

# thread for semantic learning
def semLearnCore (pro, llmMngt, randomGen=False):
    proProfile = pro.getStcProfile()
    isBase64   = pro.isEncodingBase64 ()
    model = llmMngt.getActiveModel ()
    totalPathNum = 0
    subGraphNo   = 0
    maxMem       = 0
    while True:
        FGDot = semgdump.PopFuncGroup ()
        if os.path.exists (FGDot):
            paths = semcore.GetSemanticPaths(FGDot, randomGen)
            subGraphNo += 1
            #os.remove(FGDot)
            mem = getMemoryUsage()
            if mem > maxMem:
                maxMem = mem
            log_message_0 = f"[{subGraphNo}]Pop one dot -> {FGDot}, pathNum: {len (paths)} / {totalPathNum+len (paths)}, memory usage: {mem:.2f} / {maxMem:.2f} MB"
            print (log_message_0)
            with open("stat_subgraph.log", "a") as logfile:
                    logfile.write(log_message_0 + "\n")

            pathNo   = -1
            pathNum  = len (paths)
            resHash  = 0
            resLen   =0 
            for path in paths:
                pathNo += 1
                totalPathNum += 1

                try:
                    with open(path, 'r', encoding='utf-8') as file:
                        content = file.read()
                except Exception as e:
                    print(f"load the path file error:{e}\n")
                    continue
                
                #os.remove(path)
                system_prompt = f"You are a tester for {str(proProfile)}. You need to generate **diverse** test inputs with composite requirements to test this project."
                question = pro.getDynProfile(content) + f", ensure being different from last generation (djb2-hash={resHash}, len={resLen})"

                timeCost = getTimeCost ()
                print (f">>>[{timeCost}] query -->[{pathNo} / {pathNum}] @ {FGDot} - {totalPathNum}\n")
                print (f">>>path-length = {len(content)}, user_prompt-length = {len(question) - len(content)}, system_prompt-length = {len(system_prompt)}\n")

                res = model.query(question, system_prompt = system_prompt) 
                if res == None:
                    continue
                if 'result' in res:
                    resData = str(res['result'])
                    resLen  = len (resData)
                    if not isBase64:
                        resHash = semcore.saveSemTest(os.path.basename(path), resData)
                    else:
                        resHash = saveBinary (os.path.basename(path), resData)
                    #print(f"----------------------")
                    #print(f"the result: \n{res}\n")
                    #print(f"----------------------")

#test func: write the empty result to txt file
def write_if(res, func, summary):
    filename = "all_semantic.txt"
    content = f" func name: \n{func}, \n semantic: \n{res}, \n summary: \n{summary}\n"
    if res == None or len(str(res))<5:
        content = content + "-----res error----- \n\n"
    with open(filename, 'a') as file:
        file.write(content)

# get the functionality of functions
def GetFuncSemantic (pro, llmMngt):
    proProfile = pro.getStcProfile ()
    model = llmMngt.getActiveModel ()
    # get func list
    func_list = semcore.GetFuncList()
    #print(f"func_list:{func_list}")
    for i, func in enumerate(func_list):
        print(f"Processing index={i}/{len(func_list)}, current function: {func}")
        summary = semcore.getFuncSummary(func)
        print(f"summary:{summary}")
        question = f"Given {proProfile} Using no more than 16 words without subject, describe the functionality (description should be crunchy and short) of the following function: \n{summary}"
        
        timeCost = getTimeCost ()
        print (f">>>[{timeCost}] query --> {question}")
        res = model.query(question) 
        #test for empty result
        if isinstance(res,str):
            pass
        else:
            res = str(res['result'])

        write_if(res, func, summary)
        #count #token
        tokenizer = GPT2Tokenizer.from_pretrained("gpt2")
        encoded_text = tokenizer.encode(res+summary)
        token_count = len(encoded_text)
        print(f"the # tokens: {token_count}\n")
        semcore.setFuncTokenNum(func, token_count)
        print(f"After querying index={i}, result={res}")
        print("File written successfully.")
        semcore.setFuncSemantic(func, res)
    semcore.saveSummary()

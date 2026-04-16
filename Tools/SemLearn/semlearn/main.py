import os
import sys
import argparse
import signal
import threading
from .llm_handler.llm_manage import LLMManage
from .project import Project
from .fuzzer import *
from .semlearn import *
from .seed_to_graph import Seed2Graph

llmMngt = LLMManage ()

def handle_sigint(signum, frame):
    print("\nSIGINT received (Ctrl+C). Cleaning up...")
    killFuzzProcess ()
    delShm ()
    semgdump.Exit ()
    exit(0)

def CheckFileExist(Path, compress=".tar.gz"):
    if not os.path.isfile(Path):
        # try tar.gz
        cpsPath = Path + compress
        if not os.path.isfile(cpsPath):
            print (f"{Path} is not existed, please check the input!")
            sys.exit (1)
        else:
            print (f"Decompress {cpsPath} ...")
            os.system (f"tar -xzf {cpsPath}")

#############################################
#  full version of semfuzz
#############################################
def initSemFuzz (FunctionList, SymSummary, StaticDot, pro, fuzzExe, isTimer):
    CheckFileExist (FunctionList)
    CheckFileExist (fuzzExe)
    CheckFileExist (SymSummary)
    CheckFileExist (StaticDot)

    # init semcore
    semcore.Init (SymSummary, StaticDot)

    # Init shared memroy for cache dynamic events
    initShm ()
    semgdump.Init ("CallGraph.dot", FunctionList, StaticDot)
    semgdump.StartDumping ()

    # thread: semantic path learning
    fgThread  = threading.Thread(target=semLearnCore, args=(pro,llmMngt,))
    fgThread.start ()

    # start the fuzzer
    fuzzScc = startFuzz (fuzzExe, isTimer)
    if fuzzScc == False:
        delShm ()
        semgdump.Exit ()
        
    ########################################################################################
    #cgThread.join()
    fgThread.join()
    delShm ()

#############################################
#  no-llm version of semfuzz
#############################################
def initNoLLMSemFuzz (FunctionList, StaticDot, fuzzExe, isTimer):
    CheckFileExist (FunctionList)
    CheckFileExist (fuzzExe)
    CheckFileExist (StaticDot)

    os.environ["NON_SEMANTIC_LEARNING"] = "true"

    # Init shared memroy for cache dynamic events
    initShm ()
    semgdump.Init ("CallGraph.dot", FunctionList, StaticDot)
    semgdump.StartDumping ()

    # start the fuzzer
    fuzzScc = startFuzz (fuzzExe, isTimer)
    if fuzzScc == False:
        delShm ()
        semgdump.Exit ()
        
    ########################################################################################
    while (True):
        pass

#############################################
#  no-sempath version of semfuzz
#############################################
def initNoSempathFuzz (FunctionList, StaticDot, pro, fuzzExe, isTimer):
    CheckFileExist (FunctionList)
    CheckFileExist (fuzzExe)
    CheckFileExist (StaticDot)

    os.environ["NON_SEMANTIC_LEARNING"] = "true"
    # init semcore
    semcore.Init ("None", "None")

    # Init shared memroy for cache dynamic events
    initShm ()
    semgdump.Init ("CallGraph.dot", FunctionList, StaticDot)
    semgdump.StartDumping ()

    # thread: randomly learning by LLM
    fgThread  = threading.Thread(target=semLearnNopaths, args=(pro,llmMngt,))
    fgThread.start ()

    # start the fuzzer
    fuzzScc = startFuzz (fuzzExe, isTimer)
    if fuzzScc == False:
        delShm ()
        semgdump.Exit ()
        
    ########################################################################################
    while (True):
        pass

#############################################
#  random-sempath version of semfuzz
#############################################
def initRandomSempathFuzz (FunctionList, SymSummary, StaticDot, pro, fuzzExe, isTimer):
    CheckFileExist (FunctionList)
    CheckFileExist (fuzzExe)
    CheckFileExist (SymSummary)
    CheckFileExist (StaticDot)

    # init semcore
    semcore.Init (SymSummary, StaticDot)

    # Init shared memroy for cache dynamic events
    initShm ()
    semgdump.Init ("CallGraph.dot", FunctionList, StaticDot)
    semgdump.StartDumping ()

    # thread: semantic path learning
    fgThread  = threading.Thread(target=semLearnCore, args=(pro,llmMngt,True))
    fgThread.start ()

    # start the fuzzer
    fuzzScc = startFuzz (fuzzExe, isTimer)
    if fuzzScc == False:
        delShm ()
        semgdump.Exit ()
        
    ########################################################################################
    #cgThread.join()
    fgThread.join()
    delShm ()

def helloLLM (llmMngt):
    model = llmMngt.getActiveModel ()
    res = model.query("hello, AI")
    print (f"@@helloLLM: {res}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('model', nargs='?', help='the model name to handle the request')
    parser.add_argument('arguments', nargs=argparse.REMAINDER, help='arguments to the program')

    grp = parser.add_argument_group('Main options', 'One of these (or --report) must be given')
    grp.add_argument('-a', '--auto_dir', help='specify the bench dir to automatically load all paras')
    grp.add_argument('-d', '--dynamic', action="store_true", help='enable dynamic mode')
    grp.add_argument('-t', '--timer', action="store_true", help='enable 24h timer')
    grp.add_argument('-l', '--no_llm', action="store_true", help='close semantic learning with LLM')
    grp.add_argument('-x', '--no_sem_paths', action="store_true", help='close semantic path exploration, LLM randomly gen')
    grp.add_argument('-r', '--random_sem_paths', action="store_true", help='use random path exploration')
    grp.add_argument('-s', '--seed_analysis', help='analyze the seeds generated by LLM')
    grp.add_argument('-c', '--cmd', help='command line for running the executable')
    opts = parser.parse_args()

    # default paras
    SymSummary   = "export_symbolic_summary.json" if opts.dynamic else "symbolic_summary.json"
    FunctionList = "functions_list"
    StaticDot    = "callgraph_final.dot"
    fuzzExe      = "runFuzz.sh"

    if opts.auto_dir == None:
        if opts.seed_analysis:
            opts.auto_dir = opts.seed_analysis
        else:
            print("Please specify the directory of the benchmark....")
            sys.exit (1)
    
    # init default paras with path
    AbsPath = os.path.abspath(opts.auto_dir)
    FunctionList = os.path.join(AbsPath, FunctionList)
    StaticDot    = os.path.join(AbsPath, StaticDot)
    fuzzExe      = os.path.join(AbsPath, fuzzExe)
    SymSummary   = os.path.join(AbsPath, SymSummary)
    os.chdir(opts.auto_dir)

    if opts.seed_analysis:
        initShm ()
        semgdump.Init ("CallGraph.dot", FunctionList, StaticDot)
        s2g = Seed2Graph(opts.seed_analysis, opts.cmd)
        s2g.run(semgdump)
        semgdump.Exit ()
        delShm ()
        return
    
    pro = Project ('project.yaml')
    if not pro.initProfile ():
        sys.exit (1)

    # Register the signal handler
    signal.signal(signal.SIGINT, handle_sigint)
    isTimer = True if opts.timer != None else False

    # set LLM
    if opts.model is None:
        llmMngt.showAvailableModels ()
    else:
        llmMngt.setActiveModel (opts.model)
        llmMngt.showAvailableModels ()
    helloLLM(llmMngt)

    if opts.dynamic:
        if opts.no_llm:
            initNoLLMSemFuzz (FunctionList, StaticDot, fuzzExe, isTimer)
        elif opts.no_sem_paths:
            initNoSempathFuzz (FunctionList, StaticDot, pro, fuzzExe, isTimer)
        elif opts.random_sem_paths:
            initRandomSempathFuzz (FunctionList, SymSummary, StaticDot, pro, fuzzExe, isTimer)
        else:
            initSemFuzz (FunctionList, SymSummary, StaticDot, pro, fuzzExe, isTimer)
    else:
        # init semcore
        CheckFileExist (SymSummary)
        CheckFileExist (StaticDot)  
        semcore.Init (SymSummary, StaticDot)

        GetFuncSemantic (pro, llmMngt)

if __name__ == "__main__":
    main()


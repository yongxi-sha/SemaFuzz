import os
import time
import signal
import sysv_ipc
import subprocess
import psutil
from threading import Timer
import semgdump

def backupFiles ():
    fileList = ['func_cov.txt', 'path_explore_stat.log']
    try:
        for file in fileList:
            if os.path.exists(file):
                os.rename(file, 'back_' + file)
    except Exception as e:
        pass

def killFuzzProcess(keywords=None):
    """
    Kills any process whose command line contains one of the specified keywords.
    By default, matches processes containing 'afl-fuzz' or '.cur_input'.
    """
    if keywords is None:
        keywords = ["afl-fuzz", ".cur_input"]
    
    for proc in psutil.process_iter(['pid', 'cmdline']):
        try:
            cmd = ' '.join(proc.info['cmdline'])  # Combine cmdline elements into a single string
            # Check if any of the keywords appear in the command line
            if any(k in cmd for k in keywords):
                print(f"Killing PID {proc.pid}: {cmd}")
                proc.kill()  # or proc.terminate() if you prefer a gentler approach
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            # Process no longer exists or we don't have permission
            pass

def delShm ():
    shm_key = os.getenv("SHM_QUEUE_KEY")
    if shm_key == None:
        return
    try:
        shm_key = int(shm_key, 16) if shm_key.startswith("0x") else int(shm_key)
        shm = sysv_ipc.SharedMemory(shm_key)
        shm.remove()
        print(f"Shared memory with key {hex(shm_key)} deleted successfully.")
    except Exception as e:
        print(f"Shared memory with key {hex(shm_key)} deleted failed...")

def initShm():
    # Start with a default key if SHM_QUEUE_KEY is not set
    tryNum = 0
    shm_key = 0xC3B3C5D0
    while True:
        try:
            # Check if the shared memory key is already in use
            sysv_ipc.SharedMemory(shm_key)
            print(f"[Try-{tryNum}]Shared memory key {hex(shm_key)} is already in use. Trying the next key...")
            shm_key += 1
            tryNum += 1
        except sysv_ipc.ExistentialError:
            # This key is available
            os.environ["SHM_QUEUE_KEY"] = str(hex(shm_key))
            print(f"[Try-{tryNum}]Set shared memory key as {hex(shm_key)}...")
            break

fuzzPoc = None
def exitFuzz ():
    global fuzzPoc
    curTime = time.strftime("[%Y-%m-%d %H:%M:%S]", time.localtime(time.time()))		
    print ("\n****************************************************************")
    print (f"{curTime}[SemLearn]: fuzzing execution {fuzzPoc.pid} Done! .....")
    print ("****************************************************************\n")
    if fuzzPoc != None:
        killFuzzProcess ()
        os.killpg(os.getpgid(fuzzPoc.pid), signal.SIGTERM)
    semgdump.Exit ()
    delShm ()

def startFuzz (Executable, setTimer):
    backupFiles ()
    
    global fuzzPoc        
    tryNum = 0
    while tryNum < 3:
        try:             
            fuzzPoc = subprocess.Popen(Executable, shell=True, stdout=subprocess.DEVNULL, preexec_fn=os.setsid)
            print(f"[TRY-{tryNum}]Started process [Executable] with PID: {fuzzPoc.pid}")
        except Exception as e:
            print(f"[TRY-{tryNum}]Failed to start fuzzing process: {e}")

        time.sleep (2)
        retcode = fuzzPoc.poll()
        if retcode is not None:
            print(f"[TRY-{tryNum}]Fuzzing process terminated with exit code {retcode}")
            os.system('rm -rf fuzz')
        else:
            if setTimer == True:
                timer = Timer(24 * 3600, exitFuzz)
                timer.start()
            return True 
        tryNum = tryNum + 1

    return False


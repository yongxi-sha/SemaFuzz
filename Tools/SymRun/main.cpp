/***********************************************************
 * Author: Wen Li
 * Date  : 09/30/2024
 * Describe: symbolic execution for function summarization
 * History:
   <1>  09/30/2024, create
************************************************************/
#include "sym_run.h"
#include <filesystem>

using namespace llvm;
using namespace std;
using namespace SVF;

static float  svfMemUsage = 0;
static size_t svfTimeCost = 0;

static float getMemUsage() 
{
    std::ifstream statusFile("/proc/self/status");
    std::string line;
    size_t memoryUsageMB = 0;

    if (statusFile.is_open()) {
        while (std::getline(statusFile, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line);
                std::string label, unit;
                std::cout<<line<<std::endl;
                iss >> label >> memoryUsageMB >> unit;
                break;
            }
        }
        statusFile.close();
    }

    return memoryUsageMB*1.0 / 1024;
}


static inline SvfCore* getSVFCore (string bcFile)
{
    if (!filesystem::exists(bcFile))
    {
        return NULL;
    }

    size_t m1 = getMemUsage();
    size_t t1 = time (NULL);

    vector<string> moduleVec;
    moduleVec.push_back (bcFile);
    SvfCore *svfobject = new SvfCore (moduleVec);
    assert(svfobject != NULL);

    svfMemUsage = getMemUsage() - m1;
    svfTimeCost = time (NULL) - t1;
    printf("SVF-MEM: %.1f, SVF-TIME: %lu \r\n", svfMemUsage, svfTimeCost);

    return svfobject;
}

static inline FuncIDs* getFuncIDs (string funcIDFile)
{
    if (funcIDFile == "")
    {
        return NULL;
    }

    FuncIDs* funcIDs = new FuncIDs (funcIDFile);
    assert (funcIDs != NULL);
    return funcIDs;
}

static void runOnModule(SvfCore* svfObj, FuncIDs* funcIDs)
{
    SymRun* symRunner = new SymRun (svfObj);
    assert (symRunner != NULL);

    unsigned fID = 0;
    for (Module &M : LLVMModuleSet::getLLVMModuleSet()->getLLVMModules())
    {
        for(auto it = M.begin(); it != M.end(); it++)
        {
            llvm::Function *llvmFunc = &(*it);
            if (funcIDs != NULL)
            {
                fID = funcIDs->getFuncID (llvmFunc->getName().str());
                if (fID == 0)
                {
                    continue;
                }
            }
            else
            {
                fID++;
            }

            symRunner->initSymRun (M);
            if (!symRunner->runOnFunction(*llvmFunc))
            {
                continue;
            }
            symRunner->addSummaryToJSON(fID, *llvmFunc, funcIDs); 
        }

        symRunner->addGlobalsToJSON(M);
    }
 
    symRunner->writeSummaryToJSON ();
    printf ("Memusage --> %.1f (MB)\r\n", getMemUsage() - svfMemUsage);
    
    delete symRunner;
    symRunner = NULL;
}


int main(int argc, char ** argv)
{
    switch (argc)
    {
        case 2:
        {
            SvfCore* svfObj  = getSVFCore (string (argv[1]));
            if (svfObj == NULL)
            {
                printf ("SVF is initialized failed \r\n");
                return 0;
            }

            runOnModule (svfObj, NULL);

            delete svfObj;
            break;
        }
        case 3:
        {
            FuncIDs* funcIDs = getFuncIDs (string (argv[2]));
            SvfCore* svfObj  = getSVFCore (string (argv[1]));
            if (svfObj == NULL)
            {
                printf ("SVF is initialized failed \r\n");
                return 0;
            }

            runOnModule (svfObj, funcIDs);

            delete funcIDs;
            delete svfObj;
            break;
        }
        default:
        {
            printf ("Command Formant --> SymRun [*.bc] ([function_list]) \r\n");
            return 0;
        }
    }
    
    return 0;
}


#include <semcore.h>
#include <unistd.h>

extern void Init(char* symSumPath, char* cgDotPath);
extern void GetSemSummary(char* inDot);
extern SemLearner* GetSemLearner ();

static inline void mergeSemMoudle (char *symSum)
{
    ModuleSem targetSemMd;
    ModuleSem semMd;

    targetSemMd.initModule ("export_symbolic_summary.json");
    semMd.initModule (symSum);

    for (auto it = semMd.gv_begin(); it != semMd.gv_end (); it++)
    {
        string gvName = it->first;
        targetSemMd.mergeGlobalVar (gvName, it->second);
    }

    targetSemMd.exportSemSummary ();
    return;
}

int main (int argc, char** argv)
{
    char *inDot     = NULL;
    char *symSum    = NULL;
    char *mgSymSum  = NULL;
    char *wholeCgDot= NULL;
    char * test     = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "s:d:c:t:m:")) != -1) 
    {
        switch (opt) 
        {
            case 't':
            {
                test = strdup (optarg);
                break;
            }
            case 'c':
            {
                wholeCgDot = strdup (optarg);
                break;
            }
            case 'd':
            {
                inDot = strdup (optarg);
                break;
            }
            case 's':
            {
                symSum = strdup (optarg);
                break;
            }
            case 'm':
            {
                mgSymSum = strdup (optarg);
                break;
            }
            default:
            {
                printf("Unknown option\n");
                exit (0);
            }
        }
    }

    if (test != NULL)
    {
        if (test == "all") SemTest::semTest();   
        SemLearnerTest::runTests(test, GetSemLearner (), (symSum==NULL)?"":symSum, (wholeCgDot==NULL)?"":wholeCgDot);
        return 0;
    }

    if (mgSymSum != NULL)
    {
        mergeSemMoudle (mgSymSum);
        return 0;
    }

    if (symSum == NULL || inDot == NULL || wholeCgDot == NULL)
    {
        printf("symSum [%p] or wholeCgDot[%p] or inDot[%p] must be specified!\n", symSum, wholeCgDot, inDot);
        return 0;
    }

    Init (symSum, wholeCgDot);
    GetSemSummary(inDot);

    free(wholeCgDot);
    free(symSum);
    free(inDot);

    return 0;
}

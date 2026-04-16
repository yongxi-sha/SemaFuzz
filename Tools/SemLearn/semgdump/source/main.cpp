
#include <semgdump.h>
#include <Queue.h>
#include <ctrace/Event.h>
#include <dump.h>
#include <iostream>
#include <string>

#define SOFT_DEBUG (getenv("SOFTDEBUG") != NULL)

int main (int argc, char** argv)
{
    char *StaticDot = NULL;
    char *DynDot    = NULL;
    char *FuncList  = NULL;
    unsigned MinSeq = 0;
    unsigned Convert = 0;

    int opt;
    while ((opt = getopt(argc, argv, "o:d:f:s:c")) != -1) 
    {
        switch (opt) {
            case 's':
            {
                StaticDot = strdup (optarg);
                break;
            }
            case 'f':
            {
                FuncList = strdup (optarg);
                break;
            }
            case 'c':
            {
                Convert = 1;
                break;
            }
            case 'd':
            {
                DynDot = strdup (optarg);
                break;
            }
            case '?':
                // getopt will already print an error message
                break;
            default:
                printf("Unknown option\n");
        }
    }

    if (Convert == 1)
    {
        if (StaticDot == NULL || FuncList == NULL)
        {
            printf("StaticDot [%p] | FuncList [%p] must be specified!\n", StaticDot, FuncList);
            return 0; 
        }
        ConvertDot (StaticDot, FuncList);
        return 0;
    }

    if (StaticDot == NULL || FuncList == NULL)
    {
        printf("StaticDot Dot [%p] or FuncList [%p] must be specified!\n", StaticDot, FuncList);
        return 0;
    }

    Init ((char*)"callgraph.dot", FuncList, StaticDot, MinSeq);

    StartReadShm ();
    while (1)
    {
        char* Dot = PopCallGraph ();
        if (Dot[0] != 0)
        {
            printf ("@@ Read Dot: %s \r\n", Dot);
            if (!SOFT_DEBUG)
            {
                rename (Dot, "CallGraph.dot");
            }
        }
        
        free (Dot);
    }
    
    return 0;
}

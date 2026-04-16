/***********************************************************
 * Author: Wen Li
 * Date  : 8/07/2024
 * Describe: FIFO Queue in shared memory
 * History:
   <1> 7/24/2020 , create
************************************************************/
#include <sys/shm.h>
#include "Queue_struct.h"


/////////////////////////////////////////////////////////////////////////
extern Queue* GetShmQueue ();

static inline unsigned char* GetAlignMemory ()
{
    Queue* Q = GetShmQueue ();
    if (Q == NULL)
    {
        return NULL;
    }
    return Q->AlignMemAddr;
}

#define ID2POS(ID) (ID>>3)
#define ID2OFF(ID) (ID&0x7)

unsigned GetFAlignValue (unsigned FuncID)
{
    if (FuncID == 0 || FuncID >= MAX_FUNCTION_SIZE)
    {
        return 0;
    }
    FuncID--;

    unsigned char* Am = GetAlignMemory ();
    
    unsigned Pos = ID2POS(FuncID);
    unsigned Off = ID2OFF(FuncID);

    return ((Am[Pos] >> Off) & 0x01);
}

void SetFAlignValue (unsigned FuncID)
{
    if (FuncID == 0 || FuncID >= MAX_FUNCTION_SIZE)
    {
        return;
    }
    FuncID--;

    unsigned char* Am = GetAlignMemory ();
    
    unsigned Pos = ID2POS(FuncID);
    unsigned Off = ID2OFF(FuncID);

    Am[Pos] |= (1<<Off);
    return;
}

void ResetFAlignValue (unsigned FuncID)
{
    if (FuncID == 0 || FuncID >= MAX_FUNCTION_SIZE)
    {
        return;
    }
    FuncID--;

    unsigned char* Am = GetAlignMemory ();
    
    unsigned Pos = ID2POS(FuncID);
    unsigned Off = ID2OFF(FuncID);

    Am[Pos] &= ~(1<<Off);
    return;
}

void ResetFAVAll ()
{
    unsigned char* Am = GetAlignMemory ();
    
    memset (Am, 0, MAX_ALIGN_SIZE);
    return;
}


void FATest ()
{
    ResetFAVAll ();

    unsigned ID2Value[] = {0,0,0,1,0,0,1,1,1,0,0,0,0,0,1,0,1,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1};

    // set value
    for (int id = 0; id < sizeof (ID2Value); id++)
    {
        if (ID2Value[id] == 1)
        {
            SetFAlignValue (id+1);
        }
    }

    // check value
    for (int id = 0; id < sizeof (ID2Value)/sizeof (unsigned); id++)
    {
        if (GetFAlignValue (id+1) != ID2Value[id])
        {
            fprintf (stderr, "[FATest] Failed!, Index: %u, expect value: %u but actual value: %u\r\n", id, ID2Value[id], GetFAlignValue (id+1));
        }
    }

    #include <time.h>
    #include <stdlib.h>

    srand(time(NULL));   // Initialization, should only be called once.  

    // randome test
    int RandomTime = 100;
    while (RandomTime)
    {
        int r = rand()%MAX_FUNCTION_SIZE + 1; 

        SetFAlignValue (r);
        if (GetFAlignValue (r) != 1)
        {
            fprintf (stderr, "[FATest] Failed!, Index: %u, expect value: 1 but actual value: %u\r\n", r, GetFAlignValue (r));
        }
        RandomTime--;
    }

    printf ("@@@FATest done!\r\n");
    return;
}
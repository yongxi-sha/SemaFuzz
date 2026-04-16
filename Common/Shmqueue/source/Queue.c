/***********************************************************
 * Author: Wen Li
 * Date  : 2/07/2022
 * Describe: FIFO Queue in shared memory
 * History:
   <1> 7/24/2020 , create
************************************************************/

#include <sys/shm.h>
#include "Queue_struct.h"


/////////////////////////////////////////////////////////////////////////
// Debug info
/////////////////////////////////////////////////////////////////////////
#ifdef __DEBUG__
#define DEBUG(format, ...) fprintf(stderr, "<shmQueue>" format, ##__VA_ARGS__)
#else
#define DEBUG(format, ...) 
#endif


/////////////////////////////////////////////////////////////////////////
// Global variables in local process
/////////////////////////////////////////////////////////////////////////
static Queue *g_Queue = NULL;
static int g_SharedId = 0;



static inline void* GetQueueMemory (MEMMOD MemMode, unsigned Size, char *ShareMemKey)
{
    void *MemAddr;
    
    if (MemMode == MEMMOD_SHARE)
    {
        key_t ShareKey = (key_t)strtol(ShareMemKey, NULL, 16);
        int SharedId = shmget(ShareKey, Size, 0666);
        if(SharedId == -1)
        {
            SharedId = shmget(ShareKey, Size, 0666|IPC_CREAT);
            assert (SharedId != -1);

            MemAddr = shmat(SharedId, 0, 0);
            assert (MemAddr != (void*)-1);

            memset (MemAddr, 0, Size);
        }
        else
        {
            MemAddr = shmat(SharedId, 0, 0);
            assert (MemAddr != (void*)-1);
        }

        g_SharedId = SharedId;
    }
    else
    {
        MemAddr = malloc (Size);
        assert (MemAddr != NULL);

        memset (MemAddr, 0, Size);
    }
    
    return MemAddr;
}

static inline char* GetShareKey ()
{
    char *ShareMemKey = getenv(SHM_QUEUE_KEY);
    if (ShareMemKey == NULL)
    {
        ShareMemKey = DEFAULT_SHARE_KEY;
    }

    fprintf (stderr, "@@GetShareKey -> %s\r\n", ShareMemKey);
    return ShareMemKey;
}


static inline unsigned GetQueueCap ()
{
    unsigned QueueNum;

    char *ShmQCap = getenv(SHM_QUEUE_CAP);
    if (ShmQCap == NULL)
    {
        QueueNum = DEFAULT_QUEUE_SIZE;
    }
    else
    {
        QueueNum = (unsigned)atoi (ShmQCap);
        QueueNum = (QueueNum < DEFAULT_QUEUE_SIZE) ? DEFAULT_QUEUE_SIZE : QueueNum;
    }

    return QueueNum;
}


void InitQueue (MEMMOD MemMode)
{
    Queue* Q;

    if (g_Queue != NULL)
    {
        DEBUG("@@@@@ Warning: Repeat comimg into InitQueue: %p-%u\r\n", g_Queue, g_SharedId);
        return;
    }

    char *ShareMemKey = GetShareKey ();
    unsigned QueueNum = GetQueueCap ();

    unsigned Size = sizeof (Queue) + QueueNum * sizeof (QNode);
    Q = (Queue *)GetQueueMemory (MemMode, Size, ShareMemKey);
    if (Q->NodeNum == 0)
    {
        DEBUG ("@@@@@ start InitQueue[%u]\r\n", QueueNum);
        Q->NodeNum    = QueueNum;
        Q->MaxNodeNum = 0;
        Q->ExitFlag   = 0;
        Q->MemMode    = MemMode;
        Q->FailNum    = 0;
        Q->QCmp       = NULL;

        pthread_rwlockattr_t LockAttr;
        pthread_rwlockattr_setpshared(&LockAttr, PTHREAD_PROCESS_SHARED);

        process_lock_init(&Q->QLock, &LockAttr);
        process_lock_init(&Q->QSlideWindow.WLock, &LockAttr);
    }

    g_Queue = Q;

    DEBUG ("InitQueue:[%p]-[%u] ShareMemKey = %s\r\n", Q, QueueNum, ShareMemKey);
    return;
}


void ClearQueue ()
{
    if (g_Queue == NULL)
    {
        char *ShareMemKey = GetShareKey ();
        unsigned QueueNum = GetQueueCap ();
        unsigned Size = sizeof (Queue) + QueueNum * sizeof (QNode);
        Queue *Q = (Queue *)GetQueueMemory (MEMMOD_SHARE, Size, ShareMemKey);
        assert (Q != NULL);

        if (Q->NodeNum == 0)
        {
            return;
        }

        g_Queue = Q;    
    }
    
    Queue* Q = g_Queue;
    process_lock(&Q->QLock);
    //Q->MaxNodeNum = 0;
    Q->ExitFlag   = 0;
    Q->Hindex = Q->Tindex = 0;
    //memset (Q_2_NODELIST(Q), 0, Q->NodeNum * sizeof (QNode));
    process_unlock(&Q->QLock);
    
    return;
}

static inline QNode* AllocNode (Queue* Q)
{
    QNode* Node = NULL;

    process_lock(&Q->QLock);
    if ((Q->Tindex+1)%Q->NodeNum != Q->Hindex)
    {
        Node = Q_2_NODELIST(Q) + Q->Tindex;
        Node->IsReady = 0;

        Q->Tindex++;
        if (Q->Tindex >= Q->NodeNum)
        {
            Q->Tindex = 0;
        }
    }
    else
    {
        Q->FailNum++;
    }
    //DEBUG ("InQueue: [%p][%u, %u]/%u \r\n", Q, Q->Hindex, Q->Tindex, Q->NodeNum);
    process_unlock(&Q->QLock);
    
    return Node;
}

static inline QNode* SpinAllocNode (Queue* Q)
{
    QNode *NewNode;
    do
    {
        NewNode = AllocNode (Q);
    } while (NewNode == NULL);

    return NewNode;
}

Queue* GetShmQueue ()
{
    if (g_Queue == NULL)
    {
        InitQueue (MEMMOD_SHARE);
        assert (g_Queue != NULL);
    }

    return g_Queue;
}

QNode* InQueue ()
{ 
    Queue* Q = GetShmQueue ();

    return SpinAllocNode (Q);
}

static inline void SetNodeData (QNode* N, unsigned Key, Q_SetData QSet, void *Data)
{
    N->TimeStamp = time (NULL);
    N->TrcKey = (unsigned short)Key;

    if (QSet != NULL)
    {
        QSet (N, Data);
        N->IsReady = 1;
    }
    else
    {
        N->IsReady = 0;
    }

    return;
}

/////////////////////////////////////////////////////////////////////////
// OPT by slide windows
/////////////////////////////////////////////////////////////////////////
static inline unsigned IsRepeatedInterval (QWindow* QW, Q_Cmpata QCmp, unsigned Interval)
{
    unsigned Index = QW->wHIndex;
    for (unsigned i = 0; i < Interval; i++)
    {
        if (QCmp (QW->qWindows + Index%WIN_SIZE, 
                  QW->qWindows + (Index + Interval)%WIN_SIZE) == 0)
        {
            return 0;
        }
        Index++;
    }

    return 1;
}

static inline void OptBySlideWindows2 (QWindow* QW, QNode *N, Q_Cmpata QCmp)
{
    process_lock(&QW->WLock);
    if ((QW->wTIndex+1)%WIN_SIZE != QW->wHIndex)
    {
        memcpy (QW->qWindows + QW->wTIndex, N, sizeof (QNode));
        QW->wTIndex = (QW->wTIndex + 1)%WIN_SIZE;
    }

    if ((QW->wTIndex+1)%WIN_SIZE != QW->wHIndex)
    {
        process_unlock(&QW->WLock);
        return;
    }

    unsigned Interval = MAX_INTERVAL;
    while (Interval > 0)
    {
        if (IsRepeatedInterval (QW, QCmp, Interval) != 0)
        {
            // remove the repeated INTERVAL ones
            QW->wHIndex = (QW->wHIndex + Interval)%WIN_SIZE;
            DEBUG ("[OptBySlideWindows]Interval:%u, Repeated Detected, move wHIndex -> %u\r\n", Interval, QW->wHIndex);
            return;
        }

        Interval -= 2;
    }

    QNode *NewNode = SpinAllocNode (g_Queue);
    memcpy (NewNode, QW->qWindows + QW->wHIndex, sizeof (QNode));

    // move the window
    QW->wHIndex = (QW->wHIndex + 1)%WIN_SIZE;
    DEBUG ("[OptBySlideWindows] No Repeated, move wHIndex -> %u\r\n", QW->wHIndex);
    process_unlock(&QW->WLock);

    return;
}

static inline unsigned IsRepeated (QWindow* QW, Q_Cmpata QCmp)
{
    unsigned Index = QW->wHIndex;
    for (unsigned i = 0; i < 2; i++)
    {
        if (QCmp (QW->qWindows + Index%WIN_SIZE, 
                  QW->qWindows + (Index + 2)%WIN_SIZE) == 0)
        {
            return 0;
        }
        Index++;
    }

    return 1;
}

/*
typedef struct ObjValue
{
    unsigned char Type;
    unsigned char Attr;
    unsigned short Length;
    unsigned int  Value;
} ObjValue;
*/


static inline void OptBySlideWindows (QWindow* QW, QNode *N, Q_Cmpata QCmp)
{
    process_lock(&QW->WLock);

    //ObjValue *OV = (ObjValue*)N->Buf;
    //DEBUG ("[OptBySlideWindows]wHIndex: %u, wTIndex: %u <- <%u, %u>\r\n", QW->wHIndex, QW->wTIndex, N->TrcKey, OV->Value);

    if ((QW->wTIndex+1)%WIN_SIZE != QW->wHIndex)
    {
        memcpy (QW->qWindows + QW->wTIndex, N, sizeof (QNode));
        QW->wTIndex = (QW->wTIndex + 1)%WIN_SIZE;
    }

    if ((QW->wTIndex+1)%WIN_SIZE != QW->wHIndex)
    {
        process_unlock(&QW->WLock);
        return;
    }
    
    DEBUG ("[OptBySlideWindows] slide window full, start check repeated...\r\n");
    if (IsRepeated (QW, QCmp) == 0)
    {
        QNode *NewNode = SpinAllocNode (g_Queue);
        memcpy (NewNode, QW->qWindows + QW->wHIndex, sizeof (QNode));

        // move the window
        QW->wHIndex = (QW->wHIndex + 1)%WIN_SIZE;
        DEBUG ("[OptBySlideWindows] No Repeated, move wHIndex -> %u\r\n", QW->wHIndex);
    }
    else
    {
        // remove the repeated WIN_OPT_NUM ones
        QW->wHIndex = (QW->wHIndex + 2)%WIN_SIZE;
        DEBUG ("[OptBySlideWindows] Repeated Detected, move wHIndex -> %u\r\n", QW->wHIndex);
    }

    process_unlock(&QW->WLock);
    return;
}

void InQueueKeyOpt (unsigned Key, Q_SetData QSet, Q_Cmpata QCmp, void *Data)
{
    if (g_Queue == NULL)
    {
        InitQueue (MEMMOD_SHARE);
        assert (g_Queue != NULL);
    }
    Queue* Q = g_Queue;
    if (Q->QCmp == NULL)
    {
        Q->QCmp = QCmp;
    }

#ifdef QUEUE_OPT

    QNode TmpNode;
    SetNodeData (&TmpNode, Key, QSet, Data);

    OptBySlideWindows2 (&Q->QSlideWindow, &TmpNode, QCmp);

#else

    QNode *NewNode = SpinAllocNode (Q);
    SetNodeData (NewNode, Key, QSet, Data);

#endif

    return;
}

QNode* InQueueKey (unsigned Key, Q_SetData QSet, void *Data)
{
    if (g_Queue == NULL)
    {
        InitQueue (MEMMOD_SHARE);
        assert (g_Queue != NULL);
    }
    
    Queue* Q = g_Queue;
    QNode* Node = NULL;

    process_lock(&Q->QLock);
    if ((Q->Tindex+1)%Q->NodeNum != Q->Hindex)
    {
        Node = Q_2_NODELIST(Q) + Q->Tindex;
        Node->TimeStamp = time (NULL);
        Node->TrcKey = (unsigned short)Key;
        Q->Tindex++;
        
        if (QSet != NULL)
        {
            QSet (Node, Data);
            Node->IsReady = 1;
        }
        else
        {
            Node->IsReady = 0;
        }
        

        if (Q->Tindex >= Q->NodeNum)
        {
            Q->Tindex = 0;
        }
    }
    else
    {
        Q->FailNum++;
    }
    //DEBUG ("InQueue: [%p][%u, %u]/%u \r\n", Q, Q->Hindex, Q->Tindex, Q->NodeNum);
    process_unlock(&Q->QLock);
    
    return Node;
}

QNode* FrontQueue ()
{
    Queue* Q = g_Queue;
    if (Q == NULL)
    {
        return NULL;
    }

    QNode* Node = NULL;
    process_lock(&Q->QLock);
    if (Q->Hindex != Q->Tindex)
    {
        Node = (Q_2_NODELIST(Q) + Q->Hindex);
    }
    //DEBUG ("FrontQueue: [%p][%u, %u]/%u \r\n", Q, Q->Hindex,Q->Tindex, Q->NodeNum);
    process_unlock(&Q->QLock);
   
    return Node;
}



void OutQueue (QNode* QN)
{
    Queue* Q = g_Queue;
    if (Q == NULL)
    {
        return;
    }

    process_lock(&Q->QLock);
    //DEBUG ("OutQueue:[%p] [%u, %u]/%u\r\n", Q, Q->Hindex, Q->Tindex, Q->NodeNum);
    
    Q->Hindex++;
    if (Q->Hindex >= Q->NodeNum)
    {
        Q->Hindex = 0;
    }
    QN->TrcKey  = 0;
    QN->IsReady = 0;
    QN->TimeStamp = 0;
    process_unlock(&Q->QLock);

    return;
}


unsigned QueueSize ()
{
    Queue* Q = g_Queue;
    if (Q == NULL)
    {
        return 0;
    }

    process_lock(&Q->QLock);
    unsigned Size = ((Q->Tindex + Q->NodeNum) - Q->Hindex)% Q->NodeNum;
    if (Size > Q->MaxNodeNum)
    {
        Q->MaxNodeNum = Size;
    }
    process_unlock(&Q->QLock);

    return Size;
}

static inline char* GetTimeStr (unsigned curTime)
{
    static char timeStr[100];

    time_t currentTime = (time_t)curTime;
    struct tm *localTime = localtime(&currentTime);

    // Format the time: "MM-DD HH:MM"
    strftime(timeStr, sizeof(timeStr), "[%m-%d %H:%M]", localTime);
    return timeStr;
}

void ShowQueue (unsigned Num)
{
    Queue* Q = g_Queue;
    if (Q == NULL)
    {
        return;
    }

    process_lock(&Q->QLock);

    QWindow* QW = &Q->QSlideWindow;
    unsigned CacheNum = ((QW->wTIndex + WIN_SIZE) - QW->wHIndex)%WIN_SIZE;
    printf ("[QUEUE]Slide window cache: %u [%u -> %u] (WIN_SIZE: %u)\r\n", CacheNum, QW->wHIndex, QW->wTIndex, WIN_SIZE);

    printf ("[QUEUE]HIndex: %u, TIndex: %u, Size: %u [MaxNodeNum: %u / %u(M)], FailNum: %u\r\n",
            Q->Hindex, Q->Tindex, ((Q->Tindex + Q->NodeNum) - Q->Hindex)% Q->NodeNum, Q->MaxNodeNum, Q->NodeNum/1024/1024, Q->FailNum);
    unsigned Size = ((Q->Tindex + Q->NodeNum) - Q->Hindex)% Q->NodeNum;

    for (unsigned ix = 0; ix < Size; ix++)
    {
        QNode *Node = Q_2_NODELIST(Q) + (Q->Hindex+ix)%Q->NodeNum;

        printf ("[QUEUE][%u]%s key:%u, ready:%u\r\n", (Q->Hindex+ix)%Q->NodeNum, GetTimeStr (Node->TimeStamp), Node->TrcKey, Node->IsReady);
        if (ix >= Num)
        {
            break;
        }
    }
    process_unlock(&Q->QLock);

    return;
}

void QueueExit ()
{
    if (g_Queue == NULL)
    {
        InitQueue (MEMMOD_SHARE);
        assert (g_Queue != NULL);
    }

    Queue* Q = g_Queue;
    QWindow* QW = &Q->QSlideWindow;
    if (QW->wTIndex == QW->wHIndex)
    {
        return;
    }
    unsigned CacheNum = ((QW->wTIndex + WIN_SIZE) - QW->wHIndex)%WIN_SIZE;
    printf ("[QueueExit]Slide window cache: %u [%u -> %u]\r\n", CacheNum, QW->wHIndex, QW->wTIndex);

    // remove the left repeated items
    unsigned Interval = (CacheNum>>2)<<1;
    DEBUG ("[QueueExit]Initial Interval:%u\r\n", Interval);
    while (Interval > 0)
    {
        if (IsRepeatedInterval (QW, Q->QCmp, Interval) != 0)
        {
                // remove the repeated INTERVAL ones
            QW->wHIndex = (QW->wHIndex + Interval)%WIN_SIZE;
            DEBUG ("[QueueExit]Interval:%u, Repeated Detected, move wHIndex -> %u\r\n", Interval, QW->wHIndex);
        }

        Interval -= 2;
    }

    while (QW->wTIndex != QW->wHIndex)
    {
        QNode *NewNode = SpinAllocNode (Q);
        memcpy (NewNode, QW->qWindows + QW->wHIndex, sizeof (QNode));

        QW->wHIndex = (QW->wHIndex + 1)%WIN_SIZE;
    }

    return;
}

void DelQueue ()
{
    if(g_SharedId == 0)
    {
        if (g_Queue != NULL)
        {
            free (g_Queue);
            g_Queue = NULL;
        }
    }
    else
    {
        if(shmdt(g_Queue) == -1)
        {
            printf("shmdt failed\n");
            return;
        }

        if(shmctl(g_SharedId, IPC_RMID, 0) == -1)
        {
            printf("shmctl(IPC_RMID) failed\n");
        }
    }

    return;
}




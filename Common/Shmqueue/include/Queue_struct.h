
/***********************************************************
 * Author: Wen Li
 * Date  : 2/07/2022
 * Describe: Queue.h - FIFO Queue
 * History:
   <1> 7/24/2020 , create
************************************************************/
#ifndef _QUEUE_STRUCT_H_
#define _QUEUE_STRUCT_H_
#include "Queue.h"

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////
// Lock definition
/////////////////////////////////////////////////////////////////////////
#define process_lock_t             pthread_rwlock_t
#define process_lock_init(x, attr) pthread_rwlock_init (x, attr) 
#define process_lock(x)            pthread_rwlock_rdlock (x) 
#define process_unlock(x)          pthread_rwlock_unlock (x)


/////////////////////////////////////////////////////////////////////////
// Queue Node mamagement
/////////////////////////////////////////////////////////////////////////
#define MAX_INTERVAL       (32)
#define WIN_SIZE           (MAX_INTERVAL*2)
#define MAX_ALIGN_SIZE     (16 * 1024)
#define MAX_FUNCTION_SIZE  (MAX_ALIGN_SIZE * 8)

typedef struct _QWindow_
{
    QNode qWindows[WIN_SIZE];

    unsigned wHIndex;
    unsigned wTIndex;
    process_lock_t WLock;
}QWindow;

typedef struct _Queue_
{
    unsigned NodeNum;
    unsigned Hindex;
    unsigned Tindex;
    unsigned ExitFlag;

    unsigned MemMode;
    unsigned MaxNodeNum;

    unsigned FailNum;
    unsigned Rev;
    
    QWindow QSlideWindow;
    Q_Cmpata QCmp;

    process_lock_t QLock;

    unsigned char AlignMemAddr[MAX_ALIGN_SIZE];
}Queue;


/////////////////////////////////////////////////////////////////////////
// Default parameters
/////////////////////////////////////////////////////////////////////////
#define DEFAULT_QUEUE_SIZE     (32 * 1024 * 1024)
#define DEFAULT_SHARE_KEY      ("0xC3B3C5D0")
#define Q_2_NODELIST(Q)        (QNode *)(Q + 1)

#ifdef __cplusplus
}
#endif

#endif 

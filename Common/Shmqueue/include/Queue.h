
/***********************************************************
 * Author: Wen Li
 * Date  : 2/07/2022
 * Describe: Queue.h - FIFO Queue
 * History:
   <1> 7/24/2020 , create
************************************************************/
#ifndef _QUEUE_H_
#define _QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>


#define  BUF_SIZE               (8)
#define  SHM_QUEUE_CAP          ("SHM_QUEUE_CAP")
#define  SHM_QUEUE_KEY          ("SHM_QUEUE_KEY")


typedef enum
{
    MEMMOD_HEAP  = 1,
    MEMMOD_SHARE = 2
}MEMMOD;

typedef struct _QNode_
{
    unsigned short TrcKey;
    unsigned char  IsReady;
    unsigned char  Rev;
    unsigned TimeStamp;
    char  Buf [BUF_SIZE];
}QNode;

static inline QNode* QBUF2QNODE (unsigned char *Qbuf)
{
    return (QNode*)(Qbuf - (sizeof (QNode)-BUF_SIZE));
}

void InitQueue (MEMMOD MemMode);
void ClearQueue ();

typedef void (*Q_SetData) (QNode *QN, void *Data);
typedef unsigned (*Q_Cmpata) (QNode *Q1, QNode *Q2);

void InQueueKeyOpt (unsigned Key, Q_SetData QSet, Q_Cmpata QCmp, void *Data);
QNode* InQueueKey (unsigned Key, Q_SetData QSet, void *Data);

QNode* InQueue ();
QNode* FrontQueue (void);
void OutQueue (QNode* QN);
unsigned QueueSize (void);

void QueueExit ();

void DelQueue (void);

void ShowQueue (unsigned Num);


unsigned GetFAlignValue (unsigned FuncID);
void SetFAlignValue (unsigned FuncID);
void ResetFAlignValue (unsigned FuncID);
void ResetFAVAll ();

#ifdef __cplusplus
}
#endif

#endif 

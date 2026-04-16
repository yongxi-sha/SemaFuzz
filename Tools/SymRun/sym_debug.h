/***********************************************************
 * Author: Wen Li
 * Date  : 09/30/2024
 * Describe: symbolic execution for function summarization
 * History:
   <1>  09/30/2024, create
************************************************************/
#ifndef __SYM_DEBUG_H__
#define __SYM_DEBUG_H__

#ifdef SEMSLICE_DEBUG
    #define DEBUG_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define DEBUG_COUT(...) (cout << __VA_ARGS__ << endl)
#else
    #define DEBUG_PRINTF(fmt, ...)
    #define DEBUG_COUT(...)
#endif


#endif
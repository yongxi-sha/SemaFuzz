#ifndef __SEM_DEBUG_H__
#define __SEM_DEBUG_H__

#if 0
    using namespace std;

    #define DEBUG_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define DEBUG_COUT(...) (cout << __VA_ARGS__ << endl)
#else
    #define DEBUG_PRINTF(fmt, ...)
    #define DEBUG_COUT(...)
#endif


#endif
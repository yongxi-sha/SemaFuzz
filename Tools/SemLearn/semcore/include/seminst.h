#ifndef __SEMINST_H__
#define __SEMFUNC_H__
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include "semdebug.h"

using namespace std;

class InstSem 
{
public:
    static string getInstSem (const string& inst)
    {
        return handleInst (inst);
    }

private:
    static inline bool isInstType(const string& str, const string& prefix) 
    {
        return str.rfind(prefix, 0) == 0;
    }

    static inline string handleInst (const string& inst)
    {
        return inst;
    }
};

#endif

#include <stdio.h>

int globalVar = 100;

int testGlobalVar(int x) {
    globalVar += x;
    return globalVar;
}

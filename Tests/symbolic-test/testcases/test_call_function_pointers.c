#include <stdio.h>

typedef int (*FunctionPtr)(int);


// Function 1 to be called via function pointer
int myFunction1(int x) {
    return x * 2;
}

// Function 2 to be called via function pointer
int myFunction2(int x) {
    return (x + 100);
}


FunctionPtr call_target = NULL;

void setCallee (FunctionPtr callee)
{
    call_target = callee;
}

int testCallFunctionPointerWithOutput(int value) 
{
    if (value%2) {
        setCallee (myFunction1);
    }
    else {
        setCallee (myFunction2);
    }
        
    return call_target (value);
}


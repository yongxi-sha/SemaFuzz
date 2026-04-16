#include <stdio.h>

typedef void (*FunctionPtr)(int, int*);


// Function 1 to be called via function pointer
void myFunction1(int x, int* out) {
    *out =  x * 2;
}

// Function 2 to be called via function pointer
void myFunction2(int x, int* out) {
    *out = (x + 100);
}


FunctionPtr call_target = NULL;

void setCallee (FunctionPtr callee)
{
    call_target = callee;
}

void testCallFunctionPointerWithOutput(int value, int* out) 
{
    if (value%2) {
        setCallee (myFunction1);
    }
    else {
        setCallee (myFunction2);
    }
        
    call_target (value, out);
}


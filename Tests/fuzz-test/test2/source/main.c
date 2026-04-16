#include <stdio.h>
#include <stdlib.h>

unsigned FuncA (unsigned char Value)
{
    unsigned Pwd = 0;
    /* y = x*x + 5x + 1*/
    unsigned FValue = Value/2 + 10*Value - 5;
    return FValue;
}


unsigned FuncB (unsigned char Value)
{
    unsigned Pwd = 0;
    /* y = x*x + 5x + 1*/
    unsigned FValue = Value *Value + 5*Value - 100;
    return FValue;
}


int main(int argc, char ** argv) 
{
    if (argc == 1)
    {
        return FuncA (argc);
    }
    
    unsigned Value = (unsigned)atoi(argv[1]);
    unsigned Pwd = 0;

    if (Value%2)
    {
        Pwd = FuncB (Value);
    }
    else
    {
        return 0;
    }
    
    return Pwd;
}





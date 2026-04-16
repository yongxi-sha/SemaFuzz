#include <stdio.h>

void myFunction1(int x) {
    printf("myFunction1 called with x = %d\n", x);
}

void myFunction2(int y) {
    printf("myFunction2 called with y = %d\n", y);
}

void testStoreFunctionPointer(void (**funcPtr)(int), int selector) {
    if (selector == 1) {
        *funcPtr = myFunction1;
    } else {
        *funcPtr = myFunction2;
    }
}


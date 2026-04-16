
#include <stdio.h>

void *testReturnNull(int input) {
    if (input > 0) {
        return NULL;
    } else {
        return &input;
    }
}



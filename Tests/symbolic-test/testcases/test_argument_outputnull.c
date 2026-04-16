
#include <stdio.h>

void testNullHandling(int input, void **output) {
    if (input > 0) {
        *output = NULL;
    } else {
        *output = &input;
    }
}


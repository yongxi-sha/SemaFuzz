
#include <stdio.h>

void testOutputThroughArgument(int input, int *output) {
    if (input > 0) {
        *output = input * 2;
    } else {
        *output = -1;
    }
}

        
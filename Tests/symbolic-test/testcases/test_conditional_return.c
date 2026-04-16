#include <stdio.h>

int testConditionalReturn(int input) {
    if (input > 10) {
        return input * 3;
    } else {
        return -1;
    }
}
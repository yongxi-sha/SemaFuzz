#include <stdio.h>

int testSimpleForLoop(int limit) {
    int sum = 0;

    for (int i = 1; i <= limit; i++) {
        sum += i;
    }

    return sum;
}



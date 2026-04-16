
#include <stdio.h>

int testPhi(int x, int y) {
    int z;
    if (x > 0) {
        z = x + y;
    } else {
        z = x - y;
    }
    return z;
}

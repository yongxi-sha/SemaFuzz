
#include <stdio.h>

int testSimpleBranching(int x, int y) {
    int result = 0;

    if (x > 0) {
        if (y > x) {
            result = y - x;
        } else {
            result = x + y;
        }
    } else {
        if (y < 0) {
            result = x * y;
        } else {
            result = x - y;
        }
    }

    return result;
}

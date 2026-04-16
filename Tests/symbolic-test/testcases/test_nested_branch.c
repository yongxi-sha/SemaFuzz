
#include <stdio.h>

int testNestedBranch(int x, int y) {
    if (x > 0) {
        if (y > x) {
            return y - x;
        } else {
            return x + y;
        }
    } else {
        return x - y;
    }
}

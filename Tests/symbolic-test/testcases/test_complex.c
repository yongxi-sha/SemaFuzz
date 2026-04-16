
#include <stdio.h>

int testComplex(int x, int y) {
    int result = 0;
    for (int i = 0; i < x; i++) {
        if (y > i) {
            result += y - i;
        } else {
            result += i - y;
        }
    }
    return result;
}


#include <stdio.h>

void testBitcast(void *ptr) {
    int *intPtr = (int *)ptr;
    *intPtr = 42;
}

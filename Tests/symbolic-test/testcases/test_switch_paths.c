#include <stdio.h>

int testSwitchPaths(int input) {
    switch (input) {
        case 0:
            return 100;
        case 1:
            return 200;
        default:
            return -1;
    }
}
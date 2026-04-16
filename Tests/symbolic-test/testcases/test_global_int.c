#include <stdio.h>

// Define a global variable
int globalVar = 10;

// Function that uses the global variable
int processWithGlobal(int input) {
    if (input > 5) {
        return input + globalVar;
    } else {
        globalVar = input * 2;
        return globalVar;
    }
}

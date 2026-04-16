#include <stdio.h>
#include <string.h>

// Define a global constant string
const char *globalConstString = "Hello, Constant World!";

// Function that processes the constant string
int processWithConstString(const char *input) {
    if (strcmp(input, globalConstString) == 0) {
        return 0;
    } else {
        return strlen(globalConstString);
    }
}

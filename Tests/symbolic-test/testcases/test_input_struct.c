
#include <stdio.h>

// Define a structure
struct InputData {
    int field1;
    int field2;
    char flag;
};

// Function that uses the structure
int processStructInput(struct InputData *input) {
    if (input->flag == 'A') {
        return input->field1 + input->field2;
    } 
    else {
        return input->field1 * input->field2;
    }
}




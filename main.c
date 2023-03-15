#include <stdio.h>
#include "structs.h"

int value = 5;
int received_value;
Option example;

int main() {
    example = new_some((void*)&value);
    // or
    // example = new_none();
    if (example.is_some) {
        received_value = *(int*)example.value_ptr;
        printf("Received value: %d\n", received_value);
    } else {
        printf("No value received.\n");
    }
    return 0;
}
#include <stdio.h>
#include "structs.h"

int value = 5;
int received_value;
Error example;

int main() {
    example = new_ok((void*)&value);
    // or
    // example = new_err(-1, "Forced error occured for testing purposes.");
    if (example.is_ok) {
        received_value = *(int*)example.value_ptr;
        printf("Received value: %d\n", received_value);
    } else {
        printf("Error received (%d): %s\n", example.error_code, example.error_string);
    }
    return 0;
}
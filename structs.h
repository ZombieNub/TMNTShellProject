#ifndef HEADER_GUARD_STRUCTS
#define HEADER_GUARD_STRUCTS

// Certain values in the Option and Error structs do not need to be initialized depending on the "is" state.
// For instance, value_ptr doesn't need to be initialized if is_some or is_ok respectively are 0. They can be left as either NULL or garbage data,
// as they shouldn't be read in the case of them being 0.

typedef struct Option {
    int is_some; // If 1, this value "exists". If 0, this value does not exist.
    void* value_ptr; // Pointer to the real underlying value. Remember to dereference correctly, or you will have garbage data
} Option;

typedef struct Error {
    int is_ok; // If 1, the value exists. If 0, the error exists.
    void* value_ptr; // Pointer to the real underlying value. Remember to dereference correctly, or you will have garbage data
    int error_code; // Indicates the error code to be handled. Exists for the program to interact with.
    char* error_string; // The error in text form. Exists for the user to read.
} Error;

// It may seem strange to have two different structs that look so similar, but there's a point to it.
// "None" is NOT an error. None indicates that no value exists, which may be completely valid
// Distinguishing between "There is nothing" and "There is nothing when there should be something" is important
// for clear communication across the program and for us to read.
// If we want to describe "There is nothing" we should use Option
// If we want to describe "There should be something" we should use Error

// Defines the type of command read from the console
enum CommandType {
    CONSOLE,
    EXIT,
    CD,
    HISTORY,
    EMPTY, // Occurs if the user just hits enter, or only has whitespace
};

// Sometimes errors can occur from void functions. Since void means "return nothing", but Error needs returns to happen at all,
// so this struct exists to mean "I'm not intending to return anything if things went well"
// This also means no dereferencing is needed in the case of a valid return, so Blank won't ever be read anyway
typedef struct Blank {} Blank;
Blank blank;
#define BLANK (void*)&blank

// Easy ways to construct Option and Error types
Option new_some(void* value_ptr);
Option new_none();

Error new_ok(void* value_ptr);
Error new_err(int error_code, char* error_string);

// Example for using Option
/*
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
*/

// Example for using Error
/*
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
*/

#endif
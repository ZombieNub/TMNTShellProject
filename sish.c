#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "structs.h"
#include "sish.h"

char** words;
Error words_result;
char* input_str = NULL;
size_t line_size = 0;
int i;

// Error<Blank>
Error sish() {
    for (i = 0; i < 3; i++) {
        printf("sish> ");
        words_result = read_from_user();
        if (words_result.is_ok) {
            //IGNORE
        }
    }
    return new_ok(BLANK);
}

// Error<char**>
Error read_from_user() {
    getline(&input_str, &line_size, stdin);
    // For now, we'll just reprint the input to make sure it worked.
    printf("%s", input_str);

    // We have to free input_str as getline automatically constructs a buffer for us.
    free(input_str);
    input_str = NULL;

    // Technically wrong. Change this away from BLANK, or the program will not work properly when this program is completed.
    return new_ok(BLANK);
}
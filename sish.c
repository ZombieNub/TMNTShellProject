#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "structs.h"
#include "sish.h"

// Error<Blank>
Error sish() {
    Error words_result;
    char** words;
    int i, j;
    int word_count;
    for (i = 0; i < 3; i++) {
        printf("sish> ");
        words_result = read_from_user(&word_count);
        if (!words_result.is_ok) {
            return words_result;
        }
        words = *(char***)words_result.value_ptr;
        for (j = 0; j < word_count; j++) {
            printf("(%d): %s\n", j, words[j]);
        }
    }
    return new_ok(BLANK);
}

// Error<char**>
Error read_from_user(int* token_amt) {
    char** words;
    char* input_str = NULL, *dup_input_str = NULL, *string_storage = NULL;
    char* token;
    char* savetoken;
    size_t line_size = 0;
    int i, j;

    getline(&input_str, &line_size, stdin);
    // strtok_r destroys input_str, even if we "redirect" it, so we'll need to duplicate it for the first tokenization loop.
    dup_input_str = (char*) malloc(sizeof(char) * line_size);
    // The string will need to be tokenized.
    // First, we count the amount of tokens that exist so we can allocate the correct amount of space.
    for (i = 0, strcpy(dup_input_str, input_str), string_storage = dup_input_str; ; i++, string_storage = NULL) {
        token = strtok_r(string_storage, " \n", &savetoken);
        if (token == NULL) {
            break;
        }
    }
    free(dup_input_str);
    // i now holds the amount of tokens in the command
    *token_amt = i;
    words = (char**) malloc(sizeof(char*) * i);
    if (words == NULL) {
        return new_err(0, "Words failed to allocate");
    }
    // Now we can store each relevant token at the relevant position.
    for (i = 0, string_storage = input_str; ; i++, string_storage = NULL) {
        token = strtok_r(string_storage, " \n", &savetoken);
        if (token == NULL) {
            break;
        }
        for (j = 0; token[j] != '\0'; j++) {}
        words[i] = (char*) malloc(sizeof(char) * j);
        strcpy(words[i], token);
    }

    // We have to free input_str as getline automatically constructs a buffer for us.
    free(input_str);

    return new_ok((void*)&words);
}
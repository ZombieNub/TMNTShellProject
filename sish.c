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
    CommandType cmd;

    int should_continue = 1;
    while (should_continue) {
        printf("sish> ");
        words_result = read_from_user(&word_count);
        // Confirm that words is "ok"
        if (!words_result.is_ok) {
            return words_result;
        }
        // If no text has been entered, don't execute the rest of the code. Simply continue to the next loop.
        if (word_count == 0) {
            continue;
        }
        // Words has been confirmed as "ok" and has a length greater than 0. We can safely unwrap it now.
        words = *(char***)words_result.value_ptr;
        // Parse the first word of the input to idenify the type of command
        cmd = parse(words[0]);
        if (cmd == EXIT) {
            should_continue = 0;
        } else {
            printf("NOT YET IMPLEMENTED: %s\n", command_to_string(cmd));
        }
        cleanup_words(words, word_count);
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

void cleanup_words(char** words, int token_amount) {
    int i;
    for (i = 0; i < token_amount; i++) {
        free(words[i]);
    }
    free(words);
}

CommandType parse(char* command) {
    if (strcmp("exit", command) == 0) {
        return EXIT;
    } else
    if (strcmp("cd", command) == 0) {
        return CD;
    } else
    if (strcmp("history", command) == 0) {
        return HISTORY;
    } else {
        return CONSOLE;
    }
}
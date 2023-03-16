#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "structs.h"
#include "sish.h"

// Error<Blank>
Error sish() {
    Error words_result;
    Error command_result;
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
        } else
        if (cmd == CONSOLE) {
            // Pad the command with an additional NULL character, so that execvp knows the end of the arguments
            words = realloc(words, sizeof(char) * (word_count + 1));
            words[word_count] = NULL;
            command_result = command(words);
            if (command_result.is_ok) {
                
            } else {
                if (command_result.error_code == 255) {
                    printf("Something went wrong with the child process. Please try again.\n");
                } else
                if (command_result.error_code == 254) {
                    printf("Command not found. Please try again.\n");
                }
            }
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
    dup_input_str = strdup(input_str);
    // The string will need to be tokenized.
    // First, we count the amount of tokens that exist so we can allocate the correct amount of space.
    for (i = 0, string_storage = dup_input_str; ; i++, string_storage = NULL) {
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

Error command(char** command_and_args) {
    int child_exit_status;
    int execvp_result;
    pid_t fork_result = fork();
    if (fork_result == -1) {
        return new_err(errno, "Fork failure");
    }
    if (fork_result == 0) {
        // We are the child process
        int pipefd[2];
        int pipe_result = pipe(pipefd);
        if (pipe_result == -1) {
            exit(-1);
        }
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        //dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp_result = execvp(command_and_args[0], command_and_args);
        if (execvp_result == -1) {
            exit(-2);
        }
        exit(-2);
    } else {
        // We are the parent process
        wait(&child_exit_status);
        if (WIFEXITED(child_exit_status)) {
            int es = WEXITSTATUS(child_exit_status);
            if (es != 0) {
                return new_err(es, "Child exit error");
            }
        }
    }
    return new_ok(BLANK);
}
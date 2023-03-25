#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "structs.h"
#include "sish.h"
#define MAX_HISTORY_SIZE 100

char* history[MAX_HISTORY_SIZE];
int history_count = 0;

// Error<Blank>
Error sish() {
    char* input_str;
    size_t line_size = 0;
    char* commandInput;

    int should_continue = 1;
    while (should_continue) {
        line_size = 0;
        printf("sish> ");
        getline(&input_str, &line_size, stdin);
        strcpy(commandInput, input_str);
        handle_input_result = handle_input(input_str, &should_continue);
        free(input_str);
        if (!handle_input_result.is_ok) {
            return handle_input_result;
        }
    }
    return new_ok(BLANK);
}

// Error<BLANK>
Error handle_input(char* input_str, int* should_continue) {
    Error words_result;
    Error command_result;
    Error handle_input_result;
    char** words;
    int i, j;
    int word_count;
    CommandType cmd;

    words_result = read_from_user(input_str, &word_count);
    // Confirm that words is "ok"
    if (!words_result.is_ok) {
        return words_result;
    }
    // If no text has been entered, don't execute the rest of the code. Simply continue to the next loop.
    if (word_count == 0) {
        cleanup_words(words, word_count);
        return new_ok(BLANK);
    }
    // Words has been confirmed as "ok" and has a length greater than 0. We can safely unwrap it now.
    words = *(char***)words_result.value_ptr;
    add_history();
    // Parse the first word of the input to idenify the type of command
    cmd = parse(words[0]);

    // EXIT command
    if (cmd == EXIT) {
        *should_continue = 0;
    } else
    if (cmd == CONSOLE) { 
        command_result = command(words, word_count);
        if (command_result.is_ok) {
        } else {
            if (command_result.error_code == 255) {
                printf("Something went wrong with the child process. Please try again.\n");
            } else
            if (command_result.error_code == 254) {
                printf("Command not found. Please try again.\n");
            } else
            if (command_result.error_code == 1) {
                // A recoverable error has occured in the child process, so we do nothing.
            } else {
                // We cannot guarantee that the shell can keep going
                cleanup_words(words, word_count);
                return command_result;
            }
        }
    } else
    // CD command
    if (cmd == CD) {
        if (words[1] == NULL) {
            // If no directory is provided, change to the user's home directory
            char *home = getenv("HOME");
            cd(home);
        } else {
            cd(words[1]);
        }
    } else
    // HiSTORY command
    if (cmd == HISTORY) {
        if (words[1] == NULL) {
            display_history();
        } else if (strcmp(word[1], "-c") == 0) {
            clear_history();
        } else {
            execute_history(handle_input_result);
        }
    } else {
        printf("NOT YET IMPLEMENTED: %s\n", command_to_string(cmd));
    }
    cleanup_words(words, word_count);
    return new_ok(BLANK);
}

// Error<char**>
Error read_from_user(char* input_str, int* token_amt) {
    char** words;
    char* dup_input_str = NULL, *string_storage = NULL;
    char* token;
    char* savetoken;
    size_t line_size = 0;
    int i, j;

    // getline(&input_str, &line_size, stdin);
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

// Error<BLANK>
Error command(char** words, int word_count) {
    int chunk_count;
    char** chunk;
    ShellCommand* commands;
    Error run_result;
    int offset_counter = 0, has_ended = 0;
    int i;
    int pipe_store[2], pipe_result;

    // Get the amount of chunks that exist.
    while (has_ended == 0) {
        chunk = delimit_by_pipes(words, word_count, &offset_counter, &has_ended);
        free(chunk);
        chunk_count++;
    }
    
    // Allocate space for the commands
    commands = malloc(sizeof(ShellCommand) * chunk_count);
    // Fill each command with the relevant chunk
    for (i = 0, offset_counter = 0; i < chunk_count; i++) {
        commands[i].command = delimit_by_pipes(words, word_count, &offset_counter, &has_ended);
    }
    // Setting up the pipes will be tricky. We should set up n - 1 pipes, and assign them properly.
    // The great thing about looping this way is that, if we only have one command, we don't construct any pipes
    for (i = 0; i < (chunk_count - 1); i++) {
        pipe_result = pipe(pipe_store);
        // Piping could go wrong. If it does, we can't actually recover, so we should return the error
        if (pipe_result == -1) {
            // We need to clean up then return the error
            for (i = 0; i < chunk_count; i++) {
                free(commands[i].command);
            }
            free(commands);
            return new_err(0, "Pipe failure in command function");
        }
        // At this point, pipe_store[0] is a read, and pipe_store[1] is a write. We need to store them appropriately
        commands[i].stdout = pipe_store[1];
        commands[i + 1].stdin = pipe_store[0];
    }
    // Now we need to call each command
    for (i = 0; i < chunk_count; i++) {
        int should_in = 0, should_out = 0;
        if (i == 0) {
            should_in = -1;
        }
        if (i == (chunk_count - 1)) {
            should_out = -1;
        }
        run_result = run_and_wait(commands[i], should_in, should_out);
        // Running could go wrong. If it does, we can't actually recover, so we should return the error
        if (!run_result.is_ok) {
            // We need to clean up then return the error
            for (i = 0; i < chunk_count; i++) {
                free(commands[i].command);
            }
            free(commands);
            return run_result;
        }
    }
    for (i = 0; i < chunk_count; i++) {
        free(commands[i].command);
    }
    free(commands);
    return new_ok(BLANK);
}

// word_counter should begin at 0. has_ended outputs a value and thus anything can be passed in
char** delimit_by_pipes(char** words, int word_count, int* offset_counter, int* has_ended) {
    int chunk_size;
    int offset;
    int compare_result;
    int hit_pipe = -1;
    char** chunk;

    // Loop through each word in words until we hit a pipe or the end of the words.
    // We do this to count up how many words begin in each chunk
    for (offset = *offset_counter; offset < word_count; offset++) {
        int compare_result = strcmp(words[offset], "|");
        if (compare_result == 0) {
            // We have encountered a pipe
            chunk_size = offset - *offset_counter;
            hit_pipe = 0;
            *has_ended = 0;
            break;
        }
        // If we have not encountered a pipe, just continue searching for the next one
    }
    if (hit_pipe == -1) {
        // We have reached the end of the commands without hitting a pipe, therefore we need to simply return a chunk containing all words
        // from offset_counter to end of the commands
        chunk_size = word_count - *offset_counter;
        *has_ended = -1;
    }
    
    // Create a new chunk and fill it with the relevant words
    // Because this is a malloc call, it's the caller's responsibility to free the chunk after it's used
    chunk = malloc(sizeof(char*) * (chunk_size + 1));
    for (offset = 0; offset < chunk_size; offset++) {
        chunk[offset] = words[offset + *offset_counter];
    }
    chunk[chunk_size] = NULL;

    // Update the offset counter for the next iteration of this function
    *offset_counter = *offset_counter + chunk_size + 1;
    return chunk;
}

// Error<BLANK>
Error run_and_wait(ShellCommand shcmd, int should_in, int should_out) {
    /*
    int i;
    printf("Command (PS: %d, %d):", should_in, should_out);
    for (i = 0; shcmd.command[i] != NULL; i++) {
        printf(" %s", shcmd.command[i]);
    }
    printf("\n");
    */
    //return new_ok(BLANK);

    int child_exit_status;
    int execvp_result;
    pid_t fork_result = fork();
    if (fork_result == -1) {
        return new_err(errno, "Fork failure");
    }
    if (fork_result == 0) {
        // We are the child process
        if (should_in == 0) {
            dup2(shcmd.stdin, STDIN_FILENO);
            close(shcmd.stdin);
        }
        if (should_out == 0) {
            dup2(shcmd.stdout, STDOUT_FILENO);
            close(shcmd.stdout);
        }
        execvp_result = execvp(shcmd.command[0], shcmd.command);
        if (execvp_result == -1) {
            exit(-2);
        }
        exit(-2);
    } else {
        // We are the parent process
        if (should_in == 0) {
            close(shcmd.stdin);
        }
        if (should_out == 0) {
            close(shcmd.stdout);
        }
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

// uses chdir() system call to execute cd command, if invalid it will display error
int cd(char *dir) {
    if (chdir(dir) == -1) {
        perror("cd");
        return 1;
    }
    return 0;
}

// appends commands to the array and increments the count
// if the count exceeds the history size, it will remove oldest command
void add_history(char* commandInput) {
    int i;
    if (history_count >= MAX_HISTORY_SIZE) {
        free(history[0]);  // free the oldest command
        for (i = 1; i < MAX_HISTORY_SIZE; i++) {
            history[i-1] = history[i];  // shift all commands to the left
        }
    history_count--;
    }
    history[history_count++] = strdup(commandInput);  // append the new command
}

// displays the history
void display_history() {
    int i;
    for (i = 0; i < history_count; i++) {
        printf("%d %s\n", i, history[i]);
    }
}

// clears the history when history -c is used
void clear_history() {
    int i;
    for (i = 0; i < history_count; i++) {
        free(history[i]);
    }
    history_count = 0;
}


//checks if offset is valid and if so, it will execute the corresponding command
void execute_history(char* history_count) {
    Error handle_input_result;
    int offset = atoi(history_count);
    if (offset < 0 || offset >= history_count) {
        printf("Invalid offset: %s\n", arg);
        return;
    }
    handle_input(history[offset]);
    handle_input_result = handle_input(history[offset], should_continue)
    if (!handle_input_result.is_ok) {
        return handle_input_result;
    }
}


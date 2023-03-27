#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#define MAX_HISTORY_SIZE 100

// Wrapper struct for a shell command, mainly for piping.
// Should contain a command "chunk", as well as appropriate stdin and stdout pipes
// If at the beginning, should not have a stdin pipe. If at the end, should not have a stdout pipe

typedef struct ShellCommand {
    char** command;
    int stdin, stdout;
} ShellCommand;

// Certain values in the Option and Error structs do not need to be initialized depending on the "is" state.
// For instance, value_ptr doesn't need to be initialized if is_some or is_ok respectively are 0. They can be left as either NULL or garbage data,
// as they shouldn't be read in the case of them being 0.

typedef struct Option {
    int is_some; // If 1, this value "exists". If 0, this value does not exist.
    void* value_ptr; // Pointer to the real underlying value. Remember to dereference correctly, or you will have garbage data
} Option;
// It's worth noting that Option is never used in the code. It was intended for history, but wasn't actually neccessary

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
typedef enum CommandType {
    CONSOLE,
    EXIT,
    CD,
    HISTORY,
} CommandType;

char* command_to_string(CommandType cmd);

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

// Converts the command type to string, for printing and debug purposes
char* command_to_string(CommandType cmd) {
    if (cmd == CONSOLE) {
        return "Console";
    } else
    if (cmd == EXIT) {
        return "Exit";
    } else
    if (cmd == CD) {
        return "CD";
    } else
    if (cmd == HISTORY) {
        return "History";
    } else {
        // Not supposed to reach this branch
        return "I'm not exactly sure how you did this.";
    }
}

// Constructs a new "Some" variant of Option
Option new_some(void* value_ptr) {
    Option opt;
    opt.is_some = 1;
    opt.value_ptr = value_ptr;
    return opt;
}

// Constructs a new "None" variant of Option
Option new_none() {
    Option opt;
    opt.is_some = 0;
    return opt;
}

// Constructs a new "Ok" variant of Error
Error new_ok(void* value_ptr) {
    Error err;
    err.is_ok = 1;
    err.value_ptr = value_ptr;
    return err;
}

// Constructs a new "Err" variant of Error
Error new_err(int error_code, char* error_string) {
    Error err;
    err.is_ok = 0;
    err.error_code = error_code;
    err.error_string = error_string;
    return err;
}

// Header functions for the shell code.
Error sish();
Error handle_input(char* input_str, int* should_continue);
Error read_from_user(char* input_str, int* token_amt);
void cleanup_words(char** words, int token_amount);
CommandType parse(char* command);
Error command(char** words, int word_count);
char** delimit_by_pipes(char** words, int word_count, int* offset_counter, int* has_ended);
Error run_and_wait(ShellCommand shcmd, int should_in, int should_out);
Error cd(char *dir);
void clear_history();
void add_history(char* commandInput);
void display_history();

// Holds all commands entered by the user
char* history[MAX_HISTORY_SIZE];
int history_count = 0;

// Entry point for the program
int main() {
    Error program_result = sish();
    if (program_result.is_ok) {
        // Since we're ok, we just do nothing.
    } else {
        printf("Error (%d): %s\n", program_result.error_code, program_result.error_string);
    }
    return 0;
}

// Error<Blank>
// Runs the shell, prompting for input from the user, processing it, and running the relevant commands
Error sish() {
    char* input_str, *command_input;
    size_t line_size = 0;
    Error handle_input_result;

    int should_continue = 1;
    while (should_continue) {
        line_size = 0;
        // Prompt the user for input
        printf("sish> ");
        getline(&input_str, &line_size, stdin);
        // Add the command to history
        command_input = malloc(sizeof(char) * line_size);
        strcpy(command_input, input_str);
        add_history(command_input);
        // Handle the command
        handle_input_result = handle_input(input_str, &should_continue);
        // Cleanup and return errors
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
    Error cd_result;
    char** words;
    int i, j, command_offset;
    int word_count;
    CommandType cmd;

    // Convert the string to individual words
    words_result = read_from_user(input_str, &word_count);
    // Confirm that words is "ok"
    if (!words_result.is_ok) {
        return words_result;
    }
    // If no text has been entered, don't execute the rest of the code. Simply continue to the next loop.
    if (word_count == 0) {
        return new_ok(BLANK);
    }
    // Words has been confirmed as "ok" and has a length greater than 0. We can safely unwrap it now.
    words = *(char***)words_result.value_ptr;
    // Parse the first word of the input to idenify the type of command
    cmd = parse(words[0]);

    // EXIT command
    if (cmd == EXIT) {
        // Tell the sish command to stop looping
        *should_continue = 0;
    } else
    if (cmd == CONSOLE) { 
        // Run the command and handle the relevant error
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
        if (word_count == 1) {
            // If no directory is provided, change to the user's home directory
            char *home = getenv("HOME");
            cd_result = cd(home);
        } else {
            cd_result = cd(words[1]);
        }
        if (!cd_result.is_ok) {
            if (cd_result.error_code != 0) {
                cleanup_words(words, word_count);
                return cd_result;
            } else {
                // In this case, we know this is a directory not found error, which is recoverable
                printf("Directory not found.\n");
            }
        }
    } else
    // HiSTORY command
    if (cmd == HISTORY) {
        if (word_count == 1) {
            // Occurs if the input is simply "history"
            // In this case, we list all entries in history
            display_history();
        } else
        if (strcmp(words[1], "-c") == 0) {
            // Occurs if the input is simply "history -c"
            // In this case, we clear the history
            clear_history();
        } else {
            // Occurs if the input is "history X", but we don't yet know what X is
            // We need to try and convert the second argument to an integer
            // Otherwise, there is invalid input and we need to return an error
            char *endptr;
            command_offset = strtol(words[1], &endptr, 10);
            if (*endptr != '\0') {
                // If this is the case, then the conversion failed
                printf("Not a valid history index (not a number)");
            } else {
                // Offset is a valid number. We need to do boundary checking
                if (command_offset < 0) {
                    printf("Not a valid history index (must be greater than 0)");
                } else
                if (command_offset >= history_count) {
                    printf("Not a valid history index (must exist in the history)");
                } else
                {
                    // This is a valid index, so we can just call it
                    cleanup_words(words, word_count);
                    return handle_input(history[command_offset], should_continue);
                }
            }
        }

    } else {
        printf("NOT YET IMPLEMENTED: %s\n", command_to_string(cmd));
    }
    // We no longer need the command, so we should free its memory
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

    // We don't free memory, as freeing it will be the job of handle_input
    // Since no errors have occured, we simply return the words
    return new_ok((void*)&words);
}

// This cleans up each word in an array of words. Convinence function for deallocating the result of read_from_user
void cleanup_words(char** words, int token_amount) {
    int i;
    for (i = 0; i < token_amount; i++) {
        free(words[i]);
    }
    free(words);
}

// Parse the first argument to see if it's exit, cd, history, or a unix program
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
// Process one or multiple commands. Supports piping
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

// Delimits an array of words by pipe characters, returning a single chunk at a time.
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
// Forks and runs a single unix program with pipes
Error run_and_wait(ShellCommand shcmd, int should_in, int should_out) {
    /*
    The pipes for forked processes is handled by the command function, as it shouldn't be the individual
    program's responsibility. This means we have to go off of should_in and should_out and open/close
    the pipes accordingly. We can't do this in the command function, since the piping will only work correctly
    once we have forked the unix process, hence why the responsibility for closing is offloaded here, but
    the responsibility for what should be closed is handled by command
    The general rule is:
        If the program is the first one, do not close input (as it has no input pipe and would crash)
        If the program is the last one, do not close output (same as above: no output pipe)
    */

    int child_exit_status;
    int execvp_result;
    // Fork the process. The child will execute the unix program
    pid_t fork_result = fork();
    if (fork_result == -1) {
        return new_err(errno, "Fork failure");
    }
    if (fork_result == 0) {
        // We are the child process
        // If we are the first command, do not override the input pipe
        if (should_in == 0) {
            dup2(shcmd.stdin, STDIN_FILENO);
            close(shcmd.stdin);
        }
        // If we are the last command, do not override the output pipe
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
        // If we are the first command, do not attempt to close the input pipe
        if (should_in == 0) {
            close(shcmd.stdin);
        }
        // If we are the last command, do not attempt to close the output pipe
        if (should_out == 0) {
            close(shcmd.stdout);
        }
        // Wait for the child to complete the process. Read the status: if it's not 0, there was an error
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

// uses chdir() system call to execute cd command, if invalid it will return the error
Error cd(char *dir) {
    if (chdir(dir) == -1) {
        return new_err(0, "Directory not found.");
    }
    return new_ok(BLANK);
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
        printf("(%d): %s", i, history[i]);
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
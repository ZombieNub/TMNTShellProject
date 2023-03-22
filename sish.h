#ifndef HEADER_GUARD_SISH
#define HEADER_GUARD_SISH

#include "structs.h"

Error sish();
Error read_from_user(int* token_amt);
void cleanup_words(char** words, int token_amount);
CommandType parse(char* command);
Error command(char** words, int word_count);
char** delimit_by_pipes(char** words, int word_count, int* offset_counter, int* has_ended);
Error run_and_wait(ShellCommand shcmd, int should_in, int should_out);

#endif
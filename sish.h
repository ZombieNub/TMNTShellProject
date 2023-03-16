#ifndef HEADER_GUARD_SISH
#define HEADER_GUARD_SISH

#include "structs.h"

Error sish();
Error read_from_user(int* token_amt);
void cleanup_words(char** words, int token_amount);
CommandType parse(char* command);
Error command(char** command_and_args);

#endif
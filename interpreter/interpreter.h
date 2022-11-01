
#ifndef _MY_SHELL_INTERPRETER_H_
#define _MY_SHELL_INTERPRETER_H_

#include "../kv_list.h"
#include "../string_list.h"

struct string_list* tokenize_str(const char* str);
void process_command(const char* command, char **env);

#endif

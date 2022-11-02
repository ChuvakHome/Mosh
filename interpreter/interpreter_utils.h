
#ifndef _MY_SHELL_INTERPRETER_UTILS_H_
#define _MY_SHELL_INTERPRETER_UTILS_H_

#include "../string_list.h"

struct string_list* split_by_whitespaces(const char *str);

char* insert_var_values(const char *str);

char* insert_special_symbols(const char *str);

#endif

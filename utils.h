
#ifndef _MY_SHELL_UTILS_H_
#define _MY_SHELL_UTILS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "string_list.h"

#define new(type) malloc(sizeof(type))
#define new_array(type, length) malloc(sizeof(type) * ((size_t) (length)))
#define new_string(length) new_array(char, (length) + 1);

char* int_to_str(int x);

void put_exit_code_to_var_list(int exit_code);

char* realloc_and_copy(const char *src, char* dst);
char* alloc_and_copy(const char *src);

char* get_full_name(const char* filename);

char* copy_string(char *dst, const char* begin, const char* end);

char* substr(const char* s, ptrdiff_t i, size_t n);
char* substrp(const char* begin, const char* end);
char* concat(const char* s1, const char* s2);
char* concat_filename(const char* directory_name, const char* filename);

bool is_whitespace(char c);

struct string_list* split_string(const char *str, const char *delim);

bool matches_template(const char *str, const char *template);

char* strtok2(const char *str, const char *delim);

char* get_wd();

#endif

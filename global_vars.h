
#ifndef _MY_SHELL_GLOBAL_VARS_H_
#define _MY_SHELL_GLOBAL_VARS_H_

#include "kv_list.h"
#include "string_list.h"

extern char* username;
extern char* home;
extern char* pwd;
extern struct kv_list *var_list;
extern struct kv_list *special_symbols;
extern struct string_list *paths;

#endif

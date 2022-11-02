#define _GNU_SOURCE

#include <dirent.h>

#include <libgen.h>

#include <signal.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "interpreter/interpreter.h"

#include "kv_list.h"
#include "string_list.h"
#include "utils.h"

#define PATH_SEPARATOR ":"

char *username;
char *home;
char *pwd;
struct kv_list *var_list;
struct kv_list *special_symbols;
struct string_list *paths;

static void fill_paths(char* paths_str) {
    paths = string_list_create();
    char *pos;

    while ((pos = strstr(paths_str, PATH_SEPARATOR)) != NULL) {
        size_t cur_path_size = (size_t) (pos - paths_str);

        char cur_path[cur_path_size];
        strncpy(cur_path, paths_str, cur_path_size);
        cur_path[cur_path_size] = 0;

        paths->add(paths, cur_path);

        paths_str = pos + 1;
    }

    if (strlen(paths_str) > 0)
      paths->add(paths, paths_str);
}

static void config_variables() {
    username = var_list->find(var_list, "USER");
    home = var_list->find(var_list, "HOME");
    fill_paths(var_list->find(var_list, "PATH"));
}

static void config_binds() {
    rl_bind_key('\t', rl_complete);
}

static void config() {
    config_variables();
    config_binds();
}

static char* read_command(bool piped_flag) {
    char* command = NULL;
    char shell_prompt[1024];

    if (!piped_flag) {
        pwd = get_wd();
        char *working_dir_bname = basename(pwd);

        snprintf(shell_prompt, sizeof(shell_prompt), "%s %s %% ", username, working_dir_bname);
        command = readline(shell_prompt);
    }
    else {
        size_t sz = 0;
        getline(&command, &sz, stdin);
    }

    if (command != NULL) {
        size_t size = strlen(command);

        if (is_whitespace(command[size - 1]))
            command[size - 1] = 0;
    }

    return command;
}

static struct pair get_pair(const char* s) {
    const char *pos = strstr(s, "=");

    const size_t key_str_size = (size_t) (pos - s);

    char* key = new_string(key_str_size);
    key[key_str_size] = 0;
    strncpy(key, s, key_str_size);

    const size_t value_str_size = strlen(pos + 1);
    char* value = new_string(value_str_size);
    value[value_str_size] = 0;
    strncpy(value, pos + 1, value_str_size);

    return (struct pair) { .key = key, .value = value };
}

static void init_var_list(char **env) {
    var_list = kv_list_create();

    for (ptrdiff_t i = 0; env[i] != NULL; ++i) {
        var_list->add(var_list, get_pair(env[i]));
    }

    pid_t pid = getpid();

    var_list->add(var_list, (struct pair) { .key = "$", .value = int_to_str(pid) });
    var_list->add(var_list, (struct pair) { .key = "?", .value = int_to_str(0) });
    var_list->put(var_list, "OLDPWD", NULL);
}

static void handler_function(int sig) {
    puts("");
    rl_forced_update_display();
}

bool is_piped() {
    return isatty(STDIN_FILENO) == 0;
}

#include "interpreter/interpreter_utils.h"

void init_special_symbols() {
    special_symbols = kv_list_create();
    special_symbols->put(special_symbols, "~", home);
}

int main(int argc, char **argv, char **env) {
#ifndef __MY_SHELL_DEBUG__
    init_var_list(env);

    config();

    init_special_symbols();

    signal(SIGINT, handler_function);

    bool piped_flag = is_piped();

    while (!feof(stdin)) {
        char* command = read_command(piped_flag);

        if (command == NULL)
            break;

        char** cur_env = var_list->to_array(var_list, true);

        if (command[0] != 0) {
            process_command(command, cur_env);
            add_history(command);
        }

        kv_list_to_array_free(var_list, cur_env);
        free(command);
    }

    if (!piped_flag)
        puts("");

    kv_list_free(var_list);
    kv_list_free(special_symbols);
    string_list_free(paths);
#endif

    return 0;
}

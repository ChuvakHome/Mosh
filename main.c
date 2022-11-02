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
struct string_list *paths;

static void fill_paths(char* paths_str) {
    paths = string_list_create();
    char *pos;

    while ((pos = strstr(paths_str, PATH_SEPARATOR)) != NULL) {
        size_t cur_path_size = (size_t) (pos - paths_str);

        char cur_path[cur_path_size];
        strncpy(cur_path, paths_str, cur_path_size);
        cur_path[cur_path_size] = 0;
        // printf("CUR_PATH: %s %zu %zu\n", cur_path, cur_path_size, strlen(cur_path));

        paths->add(paths, cur_path);

        paths_str = pos + 1;
    }

    if (strlen(paths_str) > 0)
      paths->add(paths, paths_str);

    // puts(">> PATHS:");

    // struct string_list_node *tmp = paths->head;
    //
    // while (tmp != NULL) {
    //     printf("%s\n", tmp->str);
    //     tmp = tmp->next;
    // }
    //
    // puts("<< PATHS");
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

static void try_do_pipe() {
    pid_t pid = fork();
    int status;

    FILE *my_io = fopen("in_buffer", "w");
    my_io = freopen("in_buffer", "w+", my_io);
    int io_fd = fileno(my_io);

    // FILE *my_out = fopen("out_buffer", "w");
    // int out_fd = fileno(my_out);

    int filedes1[] = { io_fd, 1 };
    pipe(filedes1);

    // dup2(io_fd, 1);
    // int filedes2[] = { 0, io_fd };
    // pipe(filedes2);
    // dup2(0, io_fd);

    if (pid == 0) {
        execl("/bin/echo", "/bin/echo", "LOL", NULL);

        exit(-1);
    }
    else if (pid > 0) {
        do {
            waitpid(pid, &status, WUNTRACED);
        }  while (!WIFEXITED(status) && !WIFSIGNALED(status));

        rewind(my_io);
        int filedes2[] = { 0, io_fd };
        pipe(filedes2);

        pid = fork();

        if (pid == 0) {
            execl("/Users/aaamoj/utils/my_print", "/Users/aaamoj/utils/my_print", NULL);

            // puts("KEKKEKE");

            exit(-1);
        } else if (pid > 0) {
            do {
                waitpid(pid, &status, WUNTRACED);
            }  while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

        // exit(-1);

        fclose(my_io);
        // fclose(my_out);
    }
}

bool is_piped() {
    return isatty(STDIN_FILENO) == 0;
}

#include "interpreter/interpreter_utils.h"

int main(int argc, char **argv, char **env) {
//     init_var_list(env);
//
//     char *str = insert_var_values("\"LOL $PWD\"");
//
//     printf("STR: %s\n", str);

//     printf("%s\n", normalize_file_name("./interpreter/interpreter.c"));
//     printf("%s\n", normalize_file_name("."));
//
// #define __MY_SHELL_DEBUG__

#ifndef __MY_SHELL_DEBUG__
    init_var_list(env);

    config();

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
    string_list_free(paths);
#endif

    return 0;
}


#include "interpreter.h"

#include <dirent.h>

#include <errno.h>

#include <stdbool.h>
#include <stdio.h>

#include <string.h>

#include <unistd.h>

#include "global_vars.h"

#include "string_list.h"
#include "utils.h"

enum builtin_func_type {
    BUILTIN_CD = 0,
    BUILTIN_HELP,
    BUILTIN_EXIT,
    BUILTIN_VAR,
    UNKNOWN_BUILTIN
};

struct builtin_func {
    char* name;
    enum builtin_func_type type;
};

struct builtin_func builtin_func_list[] = {
    { .name = "cd", .type = BUILTIN_CD },
    { .name = "help", .type = BUILTIN_HELP },
    { .name = "exit", .type = BUILTIN_EXIT },
    { .name = "var", .type = BUILTIN_VAR },
    { .name = "N/A", . type = UNKNOWN_BUILTIN}
};

typedef int (*command_executor)(const char* command_name, char **args, char **env);

static void change_pwd() {
    pwd = var_list->find(var_list, "PWD");
    var_list->put(var_list, "OLDPWD", pwd);

    pwd = getwd(NULL);
    var_list->put(var_list, "PWD", pwd);
}

static int do_cd(const char* command_name, char **args, char **env) {
    const char* arg1 = args[1];
    char* new_working_dir;

    bool oldpwd_not_set = false;
    bool print_path_flag = false;

    if (arg1 == NULL)
        new_working_dir = var_list->find(var_list, "HOME");
    else if (strcmp(arg1, "-") == 0) {
        new_working_dir = var_list->find(var_list, "OLDPWD");

        print_path_flag = true;
        oldpwd_not_set = new_working_dir == NULL;
    }
    else
        new_working_dir = get_full_name(arg1);

    int cd_result = chdir(new_working_dir);

    if (cd_result == 0) {
        if (oldpwd_not_set)
            fputs("OLDPWD not set\n", stderr);
        else {
            if (print_path_flag)
                printf("%s\n", new_working_dir);

            change_pwd();
        }
    }
    else {
        switch (errno) {
            case EACCES:
                fprintf(stderr, "%s: permission denied: %s\n", "autocd", arg1);
                break;
            case ENOENT:
                fprintf(stderr, "%s: no such file or directory: %s\n", "autocd", arg1);
                break;
            case ENOTDIR:
                fprintf(stderr, "%s: not a directory: %s\n", "autocd", arg1);
                break;
            default:
                break;
        }
    }

    return cd_result;
}

static int do_help(const char* command_name, char **args, char **env) {
    return 0;
}

static int do_exit(const char* command_name, char **args, char **env) {
    exit(0);

    return 0;
}

static int do_var(const char* command_name, char **args, char **env) {
    char* var_name = args[1];

    if (var_name[0] == '$' && var_name[1] != 0)
        var_name = var_name + 1;

    char* var_value = var_list->find(var_list, var_name);
    printf("$%s = %s\n", var_name, var_value);

    return 0;
}

static int unknown_builtin_func(const char* command_name, char **args, char **env) {
    return -1;
}

command_executor executors[] = {
    [BUILTIN_CD] = do_cd,
    [BUILTIN_HELP] = do_help,
    [BUILTIN_EXIT] = do_exit,
    [BUILTIN_VAR] = do_var,
    [UNKNOWN_BUILTIN] = unknown_builtin_func
};

static struct string_list* split_by_whitespaces(const char *str) {
    struct string_list *list = string_list_create();
    char *pos;

    while ((pos = strstr(str, " ")) != NULL) {
        char* s = substrp(str, pos);

        if (s[0] != 0)
          list->add(list, s);

        free(s);
        str = pos + 1;
    }

    if (str[0] != 0)
      list->add(list, str);

    return list;
}

static bool dir_contains_file(const char *directory_name, const char *filename) {
    FILE *file;

    if (filename[0] == '/')
        file = fopen(filename, "r");
    else {
        char *path = concat_filename(directory_name, filename);
        file = fopen(path, "r");

        free(path);
    }

    bool exists = file != NULL;

    if (exists)
      fclose(file);

    return exists;

    // DIR *dir = opendir(directory_name);
    //
    // if (dir != NULL) {
    //     struct dirent *ent;
    //
    //     while ((ent = readdir(dir)) != NULL) {
    //         if (strcmp(ent->d_name, filename) == 0) {
    //             closedir(dir);
    //             return true;
    //         }
    //     }
    //
    //     closedir(dir);
    // }
    //
    // return false;
}

static char* find_file(const char *filename) {
    struct string_list *tmp = paths->head;

    while (tmp != NULL) {
        if (dir_contains_file(tmp->str, filename))
            return tmp->str;

        tmp = tmp->next;
    }

    return NULL;
}

struct string_list* tokenize(struct string_list *words) {
    char *filename = words->head->str;
    char *dir = find_file(filename);

    if (dir != NULL) {
        char *buffer = concat_filename(dir, filename);
        words->head->set_string(words->head, buffer);
    }

    return words;
}

struct string_list* tokenize_str(const char* str) {
    struct string_list *words = split_by_whitespaces(str);

    // char *filename = words->head->str;
    // char *dir = find_file(filename, paths);
    //
    // if (dir != NULL) {
    //     char *buffer = concat_filename(dir, filename);
    //     words->head->set_string(words->head, buffer);
    // }
    //
    // return words;

    return tokenize(words);
}

static void exec_command(const char* command_name, char** new_args, char **env) {
    pid_t pid = fork();
    pid_t wpid;
    int status;

    if (pid == 0) {
        execve(new_args[0], new_args, env);

        exit(-1);
    }
    else if (pid > 0) {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        status = WEXITSTATUS(status);
        put_exit_code_to_var_list(status);
    }
}

static enum builtin_func_type find_builtin_for(const char* command) {
    ptrdiff_t i = 0;

    while (builtin_func_list[i].type != UNKNOWN_BUILTIN) {
        if (strcmp(builtin_func_list[i].name, command) == 0)
            break;

        ++i;
    }

    return builtin_func_list[i].type;
}

/*static void print_string_list(struct string_list *list) {
    struct string_list *tmp = list->head;

    while (tmp != NULL) {
        printf("THIS: [%p], HEAD: [%p], DATA: { [%p], %s } TAIL: [%p], NEXT: [%p]\n", tmp, tmp->head, tmp->str, tmp->str, tmp->tail, tmp->next);

        tmp = tmp->next;
    }
}*/

static void do_autocd(const char* filename) {
    pwd = var_list->find(var_list, "PWD");

    if (dir_contains_file(pwd, filename)) {
        DIR *dir = opendir(filename);

        if (dir != NULL) {
            closedir(dir);

            char *args[3] = { "autocd", (void*) filename, NULL };
            do_cd("autocd", args, NULL);
        }
    }
}

void process_command(const char* command, char **env) {
    struct string_list* tok_list = split_by_whitespaces(command);
    char *command_name = alloc_and_copy(tok_list->head->str);

    tokenize(tok_list);

    char **new_args = tok_list->to_array(tok_list, true);

    enum builtin_func_type type = find_builtin_for(command_name);

    if (type != UNKNOWN_BUILTIN) {
        executors[type](command, new_args, env);
    }
    else {
        exec_command(command_name, new_args, env);

        do_autocd(command_name);
    }

    free(command_name);
    string_list_to_array_free(tok_list, new_args);
    string_list_free(tok_list);
}

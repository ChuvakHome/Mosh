#define _GNU_SOURCE

#include "interpreter.h"

#include <dirent.h>

#include <errno.h>

#include <inttypes.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <unistd.h>

#include "../builtins/cd.h"

#include "../global_vars.h"

#include "../utils.h"

#include "interpreter_utils.h"

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

static struct builtin_func builtin_func_list[] = {
    { .name = "cd", .type = BUILTIN_CD },
    { .name = "help", .type = BUILTIN_HELP },
    { .name = "exit", .type = BUILTIN_EXIT },
    { .name = "var", .type = BUILTIN_VAR },
    { .name = "N/A", . type = UNKNOWN_BUILTIN}
};

enum meta_command_type {
    SUDO_META_COMMAND = 0,
    MAN_META_COMMAND,
    UNKNOWN_META_COMMAND
};

struct meta_command_func {
    char *meta_command_sign;
    char *meta_command_name;
    enum meta_command_type type;
};

static struct meta_command_func meta_command_func_list[] = {
    { .meta_command_sign = "!!", .meta_command_name = "sudo", .type = SUDO_META_COMMAND },
    { .meta_command_sign = "??", .meta_command_name = "man", .type = MAN_META_COMMAND },
    { .meta_command_sign = NULL, .meta_command_name = NULL, .type = UNKNOWN_META_COMMAND }
};

typedef int (*command_executor)(const char* command_name, char **args, char **env);

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

// static struct string_list* split_by_whitespaces(const char *str) {
    // return split_string(str, " ");
//
    /*struct string_list *list = string_list_create();
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

    return list;*/
// }

static bool deep_search_file(const char *directory_name, const char *filename) {
    DIR *dir = opendir(directory_name);

    if (dir != NULL) {
        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL) {
            if (string_equals(ent->d_name, filename)) {
                closedir(dir);
                return true;
            }
        }

        closedir(dir);
    }

    return false;
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
    else
      exists = deep_search_file(directory_name, filename);

    return exists;
}

static char* find_file(const char *filename) {
    struct string_list_node *tmp = paths->head;

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
        words->head->copy_string(words->head, buffer);
        free(buffer);
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

static uint8_t file_type(const char *directory_name, const char *filename) {
    DIR *dir = opendir(directory_name);

    char *buf = normalize_file_name(filename, NULL);

    if (dir != NULL) {
        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL) {
            if (string_equals(ent->d_name, buf)) {
                closedir(dir);
                return ent->d_type;
            }
        }

        closedir(dir);
    }

    free(buf);

    return DT_UNKNOWN;
}

static void do_autocd(const char* filename, char **env) {
    char *args[3] = { "autocd", (void*) filename, NULL };
    do_cd("autocd", args, env);

    // pwd = getwd(NULL);

    // if (strstr(filename, "*") != NULL) {
    //     DIR *dir = opendir(filename);
    //
    //     if (dir != NULL) {
    //         closedir(dir);
    //
    //         char *args[3] = { "autocd", (void*) filename, NULL };
    //         do_cd("autocd", args, env);
    //     }
    // }
}

static bool can_exec_command(const char *command_name) {
    if (command_name[0] == '/')
        return true;
    else {
        char *path_dir = find_file(command_name);

        if (path_dir == NULL)
            return false;

        uint8_t ftype = file_type(path_dir, command_name);

        return ftype == DT_REG;
    }
}

static void exec_command(const char* command_name, char** new_args, char **env) {
    pid_t pid = vfork();
    int status;

    if (pid == 0) {
        execve(new_args[0], new_args, env);

        exit(-1);
    }
    else if (pid > 0) {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        status = WEXITSTATUS(status);
        put_exit_code_to_var_list(status);
    }
}

static enum builtin_func_type find_builtin_for(const char* command) {
    ptrdiff_t i = 0;

    while (builtin_func_list[i].type != UNKNOWN_BUILTIN) {
        if (string_equals(builtin_func_list[i].name, command))
            break;

        ++i;
    }

    return builtin_func_list[i].type;
}

static bool need_replace(char *str) {
    return str[0] != '\'';
}

static bool need_replace_special_symbol(char *str) {
    return str[0] != '\'' && str[0] != '"';
}

static bool is_quote(char x) {
    return x == '\'' || x == '"';
}

static void process_list(struct string_list *tok_list) {
    struct string_list_node *node = tok_list->head;

    while (node != NULL) {
        char *s = node->str;

        if (need_replace(s))
            s = insert_var_values(s);

        if (need_replace_special_symbol(s)) {
            char *str_copy = s;
            s = insert_special_symbols(str_copy);
            free(str_copy);
        }

        node->set_string(node, s + (is_quote(s[0]) ? 1 : 0));

        node = node->next;
    }
}

static char* full_command_path(const char *command_name) {
    if (command_name == NULL)
        return NULL;

    char *file_dir = find_file(command_name);

    if (file_dir == NULL)
        return NULL;

    return concat_filename(file_dir, command_name);
}

static void autocd_autocat(const char* filename, struct string_list *tok_list, char **env) {
    uint8_t ftype = file_type(pwd, filename);

    char *command_path = NULL;
    char **new_args = NULL;

    switch (ftype) {
        case DT_REG:
            command_path = full_command_path("cat");
            tok_list->insert(tok_list, 0, command_path);
            new_args = tok_list->to_array(tok_list, true);
            exec_command("cat", new_args, env);
            break;
        case DT_DIR:
            command_path = full_command_path("cd");
            do_autocd(filename, env);
            break;
        default:
            break;
    }

    free(command_path);
    string_list_to_array_free(tok_list, new_args);
}

static char* find_meta_command(const char *meta_command_sign) {
    if (meta_command_sign == NULL)
        return NULL;

    ptrdiff_t idx = 0;

    while (meta_command_func_list[idx].type != UNKNOWN_META_COMMAND) {
        if (string_equals(meta_command_func_list[idx].meta_command_sign, meta_command_sign))
            return meta_command_func_list[idx].meta_command_name;

        idx++;
    }

    return NULL;
}

#define exec_meta_command(meta_command, tokens_list, new_args)                  \
    do {                                                                        \
        string_list_to_array_free(tokens_list, new_args);                       \
                                                                                \
        struct string_list_node *tail_node = tokens_list->tail;                 \
        tail_node->remove_node(tokens_list, tail_node);                         \
                                                                                \
        tokens_list->head->copy_string(tokens_list->head, command_name);        \
                                                                                \
        tokens_list->insert(tokens_list, 0, full_command_path(meta_command));   \
        new_args = tokens_list->to_array(tokens_list, true);                    \
                                                                                \
        exec_command(meta_command, new_args, env);                              \
    } while (0);

static struct string_list* template_matchings(const struct string_list *args_list) {
    struct string_list_node *arg = args_list->head;
    struct string_list *new_args_list = string_list_create();

    while (arg != NULL) {
        if (strstr(arg->str, "*") != NULL) {
            struct string_list *rel_list = get_relevant_directories(arg->str, false);
            new_args_list->add_all(new_args_list, rel_list);

            string_list_free(rel_list);
        }
        else {
            new_args_list->add(new_args_list, arg->str);
        }

        arg = arg->next;
    }

    return new_args_list;
}

void process_command(const char* command, char **env) {
    struct string_list* tok_list = split_by_whitespaces(command);
    process_list(tok_list);
    tok_list->remove_all(tok_list, "");

    char *command_name = alloc_and_copy(tok_list->head->str);

    tokenize(tok_list);
    struct string_list *old_tok_list = tok_list;
    tok_list = template_matchings(old_tok_list);

    char **new_args = tok_list->to_array(tok_list, true);

    enum builtin_func_type type = find_builtin_for(command_name);

    if (type != UNKNOWN_BUILTIN) {
        executors[type](command_name, new_args, env);
    }
    else {
        char *end_token = tok_list->tail->str;

        char *meta_command_name = find_meta_command(end_token);

        if (meta_command_name != NULL) {
            exec_meta_command(meta_command_name, tok_list, new_args);
        }
        else {
            if (can_exec_command(command_name))
                exec_command(command_name, new_args, env);
            else
                autocd_autocat(command_name, tok_list, env);
        }
    }

    free(command_name);
    string_list_free(old_tok_list);
    string_list_to_array_free(tok_list, new_args);
    string_list_free(tok_list);
}

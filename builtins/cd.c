
#include "cd.h"

#include <dirent.h>

#include <errno.h>

#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "../global_vars.h"

#include "../string_list.h"
#include "../utils.h"

static void change_pwd() {
    pwd = var_list->find(var_list, "PWD");
    var_list->put(var_list, "OLDPWD", pwd);

    pwd = get_wd();
    var_list->put(var_list, "PWD", pwd);
}

static struct string_list* get_files_matches_template(const char *directory_path, const char *template, bool fullname) {
    DIR *dir = opendir(directory_path);

    if (dir != NULL) {
        struct string_list *files_list = string_list_create();
        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL) {
            const char *filename = ent->d_name;
            char *fullpath = NULL;

            if (strcmp(filename, ".") != 0 && strcmp(filename, "..") != 0 && matches_template(filename, template)) {
                if (fullname)
                    fullpath = concat_filename(directory_path, filename);

                files_list->add(files_list, fullpath);
                free(fullpath);
            }
        }

        closedir(dir);

        return files_list;
    }
    else
        return NULL;
}

static struct string_list* get_relevant_directories(const char *path) {
    char *full_path = get_full_name(path);

    struct string_list *list = split_string(full_path, "/");
    struct string_list_node *list_iter = list->head;

    struct string_list *relevant_directories_list = string_list_create();
    struct string_list *copy_list = string_list_create();

    relevant_directories_list->add(relevant_directories_list, "/");

    while (list_iter != NULL) {
        char *filename_template = list_iter->str;

        // printf("&> [%p], %s, %zu\n", filename_template, filename_template, strlen(filename_template));

        copy_list->copy(copy_list, relevant_directories_list);
        relevant_directories_list->clear(relevant_directories_list);

        struct string_list_node *rel_dir_list_iter = copy_list->head;

        while (rel_dir_list_iter != NULL) {
            struct string_list *good_dirs = get_files_matches_template(rel_dir_list_iter->str, filename_template, true);
            relevant_directories_list->add_all(relevant_directories_list, good_dirs);

            rel_dir_list_iter = rel_dir_list_iter->next;
        }

        list_iter = list_iter->next;
    }

    list_iter = list->head;

    free(full_path);
    string_list_free(list);
    string_list_free(copy_list);

    return relevant_directories_list;
}

int do_cd(const char* command_name, char **args, char **env) {
    const char* arg1 = args[1];
    char* new_working_dir;

    bool oldpwd_not_set = false;
    bool print_path_flag = false;

    struct string_list *rel_dir_list = NULL;

    if (arg1 == NULL)
        new_working_dir = alloc_and_copy(var_list->find(var_list, "HOME"));
    else if (strcmp(arg1, "-") == 0) {
        new_working_dir = alloc_and_copy(var_list->find(var_list, "OLDPWD"));

        print_path_flag = true;
        oldpwd_not_set = new_working_dir == NULL;
    }
    else {
        rel_dir_list = get_relevant_directories(arg1);

        if (rel_dir_list->get_size(rel_dir_list) > 1) {
            struct string_list_node *tmp = rel_dir_list->head;

            while (tmp != NULL) {
                printf("NODE: %s\n", tmp->str);

                tmp = tmp->next;
            }

            fprintf(stderr, "%s: string is not in pwd: %s\n", command_name, rel_dir_list->head->str);
            string_list_free(rel_dir_list);

            return -1;
        }
        else if (rel_dir_list->empty(rel_dir_list))
            new_working_dir = get_full_name(arg1);
        else
            new_working_dir = alloc_and_copy(rel_dir_list->head->str);
    }

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
                fprintf(stderr, "%s: permission denied: %s\n", command_name, arg1);
                break;
            case ENOENT:
                fprintf(stderr, "%s: no such file or directory: %s\n", command_name, arg1);
                break;
            case ENOTDIR:
                fprintf(stderr, "%s: not a directory: %s\n", command_name, arg1);
                break;
            default:
                break;
        }
    }

    free(new_working_dir);
    string_list_free(rel_dir_list);

    return cd_result;
}

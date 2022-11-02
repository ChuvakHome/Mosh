
#include "utils.h"

#include <dirent.h>

#include <libgen.h>

#include <string.h>

#include <stdio.h>

#include <unistd.h>

#include "global_vars.h"

char* int_to_str(int x) {
    char* buf = new_array(char, 16);
    sprintf(buf, "%d", x);

    return buf;
}

void put_exit_code_to_var_list(int exit_code) {
    char* str = int_to_str(exit_code);
    var_list->put(var_list, "?", str);
}

char* realloc_and_copy(const char *src, char *dst) {
    if (src == NULL) {
        free(dst);
        return NULL;
    }

    size_t len = strlen(src);
    dst = realloc(dst, len + 1);
    strncpy(dst, src, len);
    dst[len] = 0;

    return dst;
}

char* alloc_and_copy(const char *src) {
    if (src == NULL)
        return NULL;

    size_t len = strlen(src);
    char *buf = new_string(len);
    strncpy(buf, src, len);
    buf[len] = 0;

    return buf;
}

char* copy_string(char *dst, const char *begin, const char *end) {
    const size_t len = (size_t) (end - begin);
    strncpy(dst, begin, len);
    dst[len] = 0;

    return dst;
}

char* substr(const char *s, ptrdiff_t i, size_t n) {
    char *buf = new_string(n);
    strncpy(buf, s + i, n);
    buf[n] = 0;

    return buf;
}

char* substrp(const char *begin, const char *end) {
    if (end - begin < 0)
        return NULL;

    char *buf = new_string(end - begin);

    return copy_string(buf, begin, end);
}

char* concat(const char *s1, const char *s2) {
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    size_t len = len1 + len2;

    char *buf = new_string(len);

    strncpy(buf, s1, len1);
    strncpy(buf + len1, s2, len2);
    buf[len] = 0;

    return buf;
}

char* concat_filename(const char *directory_name, const char *filename) {
    size_t len1 = strlen(directory_name);
    size_t len2 = strlen(filename);
    size_t len = len1 + len2;

    bool flag = false;

    if (directory_name[len1 - 1] != '/') {
        flag = true;
        len++;
    }

    char *buf = new_string(len);

    strncpy(buf, directory_name, len1);

    if (flag)
      buf[len1] = '/';

    strncpy(buf + len1 + (flag ? 1 : 0), filename, len2);
    buf[len] = 0;

    return buf;
}

bool is_whitespace(char c) {
    switch (c) {
        case ' ':
        case '\n':
        case '\t':
        case '\r':
            return true;
        default:
            return false;
    }
}

char* get_full_name(const char* filename) {
    size_t len = strlen(filename);

    if (len > 1 && filename[0] == '/')
        return alloc_and_copy(filename);

    pwd = get_wd();

    if (len <= 2) {
        if (strcmp(filename, ".") == 0)
            return alloc_and_copy(pwd);
        else if (strcmp(filename, "..") == 0) {
            return alloc_and_copy(dirname(pwd));
        }
        else if (strcmp(filename, "~") == 0)
            return alloc_and_copy(home);
    }

    return concat_filename(pwd, filename);
}

char* normalize_file_name(const char* filename, char* buffer) {
    if (filename == NULL)
        return NULL;

    struct string_list *split_list = split_string(filename, "/");
    char* normalized_path = buffer;

    if (normalized_path == NULL)
        normalized_path = new_string(strlen(filename));

    char* normalized_path_ptr = normalized_path;

    struct string_list_node *split_list_node = split_list->head;

    while (split_list_node != NULL) {
        // printf("NODE: %s\n", split_list_node->str);
        char *tmp_str = split_list_node->str;

        if (!string_equals(tmp_str, ".")) {
            size_t len = strlen(tmp_str);
            strncpy(normalized_path_ptr, tmp_str, len);
            normalized_path_ptr[len] = '/';
            normalized_path_ptr[len + 1] = 0;

            normalized_path_ptr += len + 1;
        }

        split_list_node = split_list_node->next;
    }

    normalized_path_ptr[strlen(normalized_path_ptr) - 1] = 0;

    string_list_free(split_list);

    return normalized_path;
}

struct string_list* split_string(const char *str, const char *delim) {
    if (str == NULL || delim == NULL)
        return NULL;

    size_t delim_len = strlen(delim);

    struct string_list *list = string_list_create();
    char *pos;

    while ((pos = strstr(str, delim)) != NULL) {
        char *s = substrp(str, pos);

        if (s != NULL) {
            list->add(list, NULL);
            list->tail->set_string(list->tail, s);
        }

        str = pos + delim_len;
    }

    if (str[0] != 0)
        list->add(list, str);

    return list;
}

bool matches_template(const char *str, const char *template) {
    if (strstr(template, "*") == NULL)
        return strcmp(str, template) == 0;

    struct string_list *split_list = split_string(template, "*");
    struct string_list_node *tmp = split_list->head;

    char *my_str = (void*) str;
    size_t len = strlen(template);
    bool result = true;

    if (strstr(template, "*") != template) {
        result = strstr(my_str, tmp->str) == my_str;
        tmp = tmp->next;
    }

    while (result && tmp != NULL && tmp->next != NULL) {
        char *pos = strstr(my_str, tmp->str);

        if (pos == NULL)
            result = false;

        // if ((pos - my_str) >= len)
        //     return false;

        my_str = pos + strlen(tmp->str);
        tmp = tmp->next;
    }

    if (result && tmp != NULL && tmp->str != NULL && template[len - 1] != '*') {
        size_t len1 = strlen(my_str);
        size_t len2 = strlen(tmp->str);

        if (len1 < len2)
            result = false;
        else {
            my_str += len1 - 1;
            char *tmp_str = tmp->str + len2 - 1;

            while (result && len2 > 0) {
                result = *my_str == *tmp_str;

                --my_str;
                --tmp_str;
                --len2;
            }
        }
    }

    string_list_free(split_list);

    return result;
}

char* strtok2(const char *str, const char *delim) {
    char *result = NULL;

    char tmp_delim[] = { 0, 0 };

    for (ptrdiff_t i = 0; delim[i] != 0; ++i) {
        tmp_delim[0] = delim[i];
        char *pos = strstr(str, tmp_delim);

        if (pos != NULL && (result == NULL || result > pos))
            result = pos;
    }

    return result;
}

char* get_wd() {
    #ifdef __APPLE__
        return getwd(NULL);
    #else
        return get_current_dir_name();
    #endif
}

struct string_list* get_files_matches_template(const char *directory_path, const char *template) {
    DIR *dir = opendir(directory_path);

    if (dir != NULL) {
        struct string_list *files_list = string_list_create();
        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL) {
            const char *filename = ent->d_name;
            char *path = NULL;

            // printf("PATH: %s; TEMPL: %s %s\n", filename, template, matches_template(filename, template) ? "true" : "false");

            if (!string_equals(filename, ".") && !string_equals(filename, "..") && matches_template(filename, template)) {
                path = concat_filename(directory_path, filename);

                files_list->add(files_list, path);

                free(path);
            }
        }

        closedir(dir);

        return files_list;
    }
    else
        return NULL;
}

struct string_list* get_relevant_directories(const char *path, bool fullname) {
    char *normalized_path = normalize_file_name(path, NULL);

    struct string_list *list = split_string(normalized_path, "/");
    struct string_list_node *list_iter = list->head;

    struct string_list *relevant_directories_list = string_list_create();
    struct string_list *copy_list = string_list_create();

    if (path[0] == '/')
        relevant_directories_list->add(relevant_directories_list, "/");
    else
        relevant_directories_list->add(relevant_directories_list, ".");

    while (list_iter != NULL) {
        char *filename_template = list_iter->str;

        // printf("&> [%p], %s, %zu\n", filename_template, filename_template, strlen(filename_template));

        copy_list->copy(copy_list, relevant_directories_list);
        relevant_directories_list->clear(relevant_directories_list);

        struct string_list_node *rel_dir_list_iter = copy_list->head;

        while (rel_dir_list_iter != NULL) {
            struct string_list *good_dirs = get_files_matches_template(rel_dir_list_iter->str, filename_template);
            relevant_directories_list->add_all(relevant_directories_list, good_dirs);

            string_list_free(good_dirs);
            rel_dir_list_iter = rel_dir_list_iter->next;
        }

        list_iter = list_iter->next;
    }

    if (!fullname) {
        struct string_list_node *rel_dir_list_iter = relevant_directories_list->head;

        while (rel_dir_list_iter != NULL) {
            char* copy_filename = rel_dir_list_iter->str;

            if (normalized_path[0] != '/' && strlen(copy_filename) >= 2)
                copy_filename += 2;

            rel_dir_list_iter->set_string(rel_dir_list_iter, alloc_and_copy(copy_filename));
            rel_dir_list_iter = rel_dir_list_iter->next;
        }
    }

    free(normalized_path);
    string_list_free(list);
    string_list_free(copy_list);

    return relevant_directories_list;
}

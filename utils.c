
#include "utils.h"

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
    char *buf = new_string(end - begin - 1);

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

    pwd = getwd(NULL);

    if (len <= 2) {
        if (strcmp(filename, ".") == 0)
            return alloc_and_copy(pwd);
        else if (strcmp(filename, "..") == 0) {
            return dirname(pwd);
        }
        else if (strcmp(filename, "~") == 0)
            return alloc_and_copy(home);
    }

    return concat_filename(pwd, filename);
}

struct string_list* split_string(const char *str, const char *delim) {
    if (str == NULL || delim == NULL)
        return NULL;

    size_t delim_len = strlen(delim);

    struct string_list *list = string_list_create();
    char *pos;

    while ((pos = strstr(str, delim)) != NULL) {
        char *s = substrp(str, pos);

        if (s[0] != 0)
            list->add(list, s);

        free(s);
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
    size_t len = strlen(str);

    while (tmp != NULL) {
        char *pos = strstr(my_str, tmp->str);

        if (pos == NULL)
            return false;

        // if ((pos - my_str) >= len)
        //     return false;

        my_str = pos + strlen(tmp->str);
        tmp = tmp->next;
    }

    string_list_free(split_list);

    return true;
}

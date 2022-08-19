
#ifndef _MY_SHELL_STRING_LIST_H_
#define _MY_SHELL_STRING_LIST_H_

#include <stdbool.h>
#include <stddef.h>

struct string_list;
struct string_list_node;

struct string_list_node {
    char *str;
    struct string_list_node *prev;
    struct string_list_node *next;

    void (*remove_node)(struct string_list *list, struct string_list_node *node);
    void (*set_string)(struct string_list_node *self, char *str);
    void (*copy_string)(struct string_list_node *self, const char *str);
};

struct string_list {
    struct string_list_node *head;
    struct string_list_node *tail;

    size_t size;

    void (*add)(struct string_list *self, const char *path);
    void (*add_all)(struct string_list *self, const struct string_list *from);
    void (*copy)(struct string_list *self, const struct string_list *from);
    void (*remove)(struct string_list *self, const char *value);
    char** (*to_array)(struct string_list *self, bool null_tail);
    size_t (*get_size)(const struct string_list *self);
    bool (*empty)(const struct string_list *list);
    void (*clear)(struct string_list *self);
};

struct string_list* string_list_create();

void string_list_free(struct string_list *list);

void string_list_to_array_free(const struct string_list *list, char **array);

#endif

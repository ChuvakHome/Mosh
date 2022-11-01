
#include "string_list.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "utils.h"

static void free_node(struct string_list_node *node) {
    if (node != NULL)
        free(node->str);

    free(node);
}

static void _set_head(struct string_list *list, struct string_list_node *node) {
    list->head = node;
    list->head->prev = NULL;
}

static void _set_tail(struct string_list *list, struct string_list_node *node) {
    list->tail = node;
    list->tail->next = NULL;
}

static void _string_list_node_remove_node_impl(struct string_list *list, struct string_list_node *node) {
    struct string_list_node *prev_node = node->prev;
    struct string_list_node *next_node = node->next;

    list->size--;

    if (prev_node != NULL || next_node != NULL) {
        if (prev_node != NULL) {
            prev_node->next = next_node;

            if (next_node != NULL)
                next_node->prev = prev_node;
            else
                _set_tail(list, prev_node);
        }
        else
            _set_head(list, next_node);
    }
    else {
        list->head = NULL;
        list->tail = NULL;
    }

    free_node(node);
}

static void _string_list_node_set_string_impl(struct string_list_node *self, char *s) {
    free(self->str);
    self->str = s;
}

static void _string_list_node_copy_string_impl(struct string_list_node *self, const char *s) {
    self->str = realloc_and_copy(s, self->str);
}

static void set_functions_for_node(struct string_list_node* list_node) {
    list_node->remove_node = _string_list_node_remove_node_impl;
    list_node->set_string = _string_list_node_set_string_impl;
    list_node->copy_string = _string_list_node_copy_string_impl;
}

static struct string_list_node* create_node(struct string_list *list, const char *str) {
    struct string_list_node *new_node = new(struct string_list_node);
    new_node->str = alloc_and_copy(str);
    new_node->prev = list->tail;
    new_node->next = NULL;

    if (list->tail != NULL)
        list->tail->next = new_node;

    list->tail = new_node;

    set_functions_for_node(new_node);

    return new_node;
}

static void _string_list_add_impl(struct string_list *self, const char *str) {
    if (self->head == NULL) {
        self->head = create_node(self, str);
        self->head->prev = NULL;
        self->tail->next = NULL;
    }
    else
        create_node(self, str);

    self->size++;
}

static void _string_list_insert_impl(struct string_list *self, size_t position, const char *str) {
    if (self == NULL)
        return;

    size_t list_size = self->size;

    if (position < 0 || position > list_size)
        return;

    if (position == 0) {
        if (self->head == NULL)
          _string_list_add_impl(self, str);
        else {
            struct string_list_node *new_head = new(struct string_list_node);

            new_head->str = alloc_and_copy(str);
            new_head->prev = NULL;
            new_head->next = self->head;

            set_functions_for_node(new_head);

            self->head = new_head;
        }
    }
    else if (position == list_size) {
        _string_list_add_impl(self, str);
    }
    else {
        struct string_list_node *node_at_pos = self->head;
        struct string_list_node *prev_node = NULL;

        while (position > 0) {
            prev_node = node_at_pos;
            node_at_pos = node_at_pos->next;
            --position;
        }

        struct string_list_node *new_node = new(struct string_list_node);

        new_node->str = alloc_and_copy(str);
        new_node->prev = prev_node;
        new_node->next = node_at_pos;

        prev_node->next = new_node;
        node_at_pos->prev = new_node;

        set_functions_for_node(new_node);
    }

    self->size = list_size + 1;
}

static void _string_list_add_all_impl(struct string_list *self, const struct string_list *from) {
    if (self != NULL && from != NULL) {
        struct string_list_node *tmp = from->head;

        while (tmp != NULL) {
            if (tmp->str != NULL)
                self->add(self, tmp->str);

            tmp = tmp->next;
        }
    }
}

static void _string_list_copy_impl(struct string_list *self, const struct string_list *from) {
    if (self != NULL && from != NULL) {
        self->clear(self);
        self->add_all(self, from);
    }
}

static struct string_list_node* find_node(const struct string_list *list, const char *value) {
    struct string_list_node *tmp = list->head;

    while (tmp != NULL) {
        if (tmp->str != NULL && strcmp(tmp->str, value) == 0)
            return tmp;

        tmp = tmp->next;
    }

    return NULL;
}

static void _string_list_remove_impl(struct string_list *self, const char *str) {
    struct string_list_node *node = find_node(self, str);

    if (node != NULL)
        node->remove_node(self, node);
}

static void _string_list_remove_all_impl(struct string_list *self, const char *str) {
    struct string_list_node *node;

    while ((node = find_node(self, str)) != NULL)
        node->remove_node(self, node);
}

static size_t _string_list_get_size_impl(const struct string_list *self) {
    return self->size;
}

static bool _string_list_empty_impl(const struct string_list *list) {
    return list->size == 0;
}

static char** _string_list_to_array_impl(struct string_list *self, bool null_tail) {
    char **arr = new_array(char*, self->get_size(self) + (null_tail ? 1 : 0));

    ptrdiff_t pos = 0;
    struct string_list_node *tmp = self->head;

    while (tmp != NULL) {
        arr[pos++] = alloc_and_copy(tmp->str);

        tmp = tmp->next;
    }

    if (null_tail)
      arr[pos] = NULL;

    return arr;
}

static void _string_list_clear_impl(struct string_list *self) {
    if (self == NULL)
        return;

    struct string_list_node *tmp = self->head;

    while (tmp != NULL) {
        struct string_list_node *remove_node = tmp;
        tmp = tmp->next;
        free_node(remove_node);
    }

    self->head = NULL;
    self->tail = NULL;
    self->size = 0;
}

struct string_list* string_list_create() {
    struct string_list *result = new(struct string_list);

    result->head = NULL;
    result->tail = NULL;
    result->size = 0;

    result->insert = _string_list_insert_impl;
    result->add = _string_list_add_impl;
    result->add_all = _string_list_add_all_impl;
    result->copy = _string_list_copy_impl;
    result->remove = _string_list_remove_impl;
    result->remove_all = _string_list_remove_all_impl;
    result->to_array = _string_list_to_array_impl;
    result->get_size = _string_list_get_size_impl;
    result->clear = _string_list_clear_impl;
    result->empty = _string_list_empty_impl;

    return result;
}

void string_list_free(struct string_list *list) {
    if (list == NULL)
        return;

    list->clear(list);
    free(list);
}

void string_list_to_array_free(const struct string_list *list, char **array) {
      if (array == NULL)
          return;

      for (ptrdiff_t i = 0; i < list->get_size(list); ++i) {
          free(array[i]);
      }

      free(array);
}

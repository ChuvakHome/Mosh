
#include "string_list.h"

#include <string.h>

#include "utils.h"

static void _string_list_add_impl(struct string_list *self, const char *str) {
    if (self->head == NULL) {
      self->head = self;
      self->str = alloc_and_copy(str);
      self->tail = self->head;
      self->tail->next = NULL;
    }
    else {
      self = self->head;

      self->tail->next = new(struct string_list);
      self->tail = self->tail->next;
      self->tail->str = alloc_and_copy(str);
      self->tail->head = self->head;
      self->tail->tail = self->tail;
      self->tail->next = NULL;
    }

    self->head->size++;
}

static char** _string_list_to_array_impl(struct string_list *self, bool null_tail) {
    char **arr = new_arr(char*, self->get_size(self) + (null_tail ? 1 : 0));

    ptrdiff_t pos = 0;
    struct string_list *tmp = self->head;

    while (tmp != NULL) {
        arr[pos++] = alloc_and_copy(tmp->str);

        tmp = tmp->next;
    }

    if (null_tail)
      arr[pos] = NULL;

    return arr;
}

static void _string_list_set_string_impl(struct string_list *self, char *s) {
    free(self->str);
    self->str = s;
}

static void _string_list_copy_string_impl(struct string_list *self, const char *s) {
    // free(self->str);
    //
    // size_t len = strlen(s);
    //
    // self->str = new_arr(char, len);
    // strncpy(self->str, s, len);
    // self->str[len] = 0;

    self->str = realloc_and_copy(s, self->str);
}

static size_t _string_list_get_size_impl(const struct string_list *self) {
    return self->head->size;
}

struct string_list* string_list_create() {
    struct string_list *result = new(struct string_list);

    result->head = NULL;
    result->tail = NULL;
    result->str = NULL;
    result->next = NULL;
    result->size = 0;

    result->add = _string_list_add_impl;
    result->to_array = _string_list_to_array_impl;
    result->set_string = _string_list_set_string_impl;
    result->copy_string = _string_list_copy_string_impl;
    result->get_size = _string_list_get_size_impl;

    return result;
}

void string_list_free(struct string_list *list) {
    if (list == NULL)
        return;

    struct string_list *tmp = list->head;

    while (tmp != NULL) {
        struct string_list *remove = tmp;
        tmp = tmp->next;

        free(remove->str);
        free(remove);
    }
}

void string_list_to_array_free(const struct string_list *list, char **array) {
    if (array == NULL)
        return;

    for (ptrdiff_t i = 0; i < list->get_size(list); ++i) {
        free(array[i]);
    }

    free(array);
}

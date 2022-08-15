
#include "kv_list.h"

#include <string.h>
#include <stdlib.h>

#include "utils.h"

void pair_free(struct pair p) {
    free(p.key);
    free(p.value);
}

static char* stringify_pair(struct pair p) {
    char* buf = NULL;

    if (p.key != NULL) {
        size_t len1 = strlen(p.key);
        size_t len2 = 0;

        if (p.value != NULL)
            len2 = strlen(p.value);

        size_t len = len1 + len2 + 1;

        buf = new_string(len);
        strncpy(buf, p.key, len1);
        buf[len1] = '=';

        if (len2 > 0)
          strncpy(buf + len1 + 1, p.value, len2);

        buf[len] = 0;
    }

    return buf;
}

static size_t _kv_list_get_size_impl(const struct kv_list *self) {
    return self->head->size;
}

static char** _kv_list_to_array_impl(struct kv_list *self, bool null_tail) {
    char** array = new_array(char*, self->get_size(self) + (null_tail ? 1 : 0));

    ptrdiff_t pos = 0;
    struct kv_list *tmp = self->head;

    while (tmp != NULL) {
        array[pos++] = stringify_pair(tmp->data);

        tmp = tmp->next;
    }

    if (null_tail)
        array[pos] = NULL;

    return array;
}

static void _kv_list_add_impl(struct kv_list *self, struct pair p) {
    struct kv_list *node = self->find_node(self, p.key);

    if (node != NULL || p.key == NULL)
      return;

    if (self->head == NULL) {
      self->head = self;
      self->data.key = alloc_and_copy(p.key);
      self->data.value = alloc_and_copy(p.value);
      self->tail = self->head;
      self->tail->next = NULL;
    }
    else {
      self = self->head;
      self->tail->next = new(struct kv_list);

      self->tail = self->tail->next;

      self->tail->data.key = alloc_and_copy(p.key);
      self->tail->data.value = alloc_and_copy(p.value);

      self->tail->head = self->head;
      self->tail->tail = self->tail;
      self->tail->next = NULL;
    }

    self->head->size++;
}

static struct kv_list* _kv_list_find_node_impl(struct kv_list *self, const char *key) {
    struct kv_list *tmp = self->head;

    while (tmp != NULL) {
      if (strcmp(tmp->data.key, key) == 0)
        return tmp;

      tmp = tmp->next;
    }

    return NULL;
}

static char* _kv_list_find_impl(struct kv_list *self, const char *key) {
    struct kv_list *tmp = self->head;

    while (tmp != NULL) {
      if (strcmp(tmp->data.key, key) == 0)
        return tmp->data.value;

      tmp = tmp->next;
    }

    return NULL;
}

static void _kv_list_put_impl(struct kv_list *self, const char *key, const char *value) {
    struct kv_list *node = self->find_node(self, key);

    if (node != NULL) {
        // free(node->data.value);
        // node->data.value = alloc_and_copy(value);
        node->data.value = realloc_and_copy(value, node->data.value);
    }
    else
      self->add(self, (struct pair) { .key = (void*) key, .value = (void*) value });
}

static void _kv_list_foreach_impl(struct kv_list *self, consumer f) {
    const struct kv_list *tmp = self->head;

    while (tmp != NULL) {
        f(tmp);

        tmp = tmp->next;
    }
}

struct kv_list* kv_list_create() {
    struct kv_list *result = new(struct kv_list);
    result->head = NULL;
    result->tail = NULL;
    result->next = NULL;

    result->data = (struct pair) { .key = NULL, .value = NULL };
    result->size = 0;

    result->add = _kv_list_add_impl;
    result->find_node = _kv_list_find_node_impl;
    result->find = _kv_list_find_impl;
    result->put = _kv_list_put_impl;
    result->foreach = _kv_list_foreach_impl;
    result->to_array = _kv_list_to_array_impl;
    result->get_size = _kv_list_get_size_impl;

    return result;
}

void kv_list_free(struct kv_list* list) {
    struct kv_list *tmp = list->head;

    while (tmp != NULL) {
        struct kv_list *remove = tmp;
        tmp = tmp->next;
        pair_free(remove->data);
        free(remove);
    }
}

void kv_list_to_array_free(const struct kv_list *list, char **array) {
    if (array == NULL)
        return;

    for (ptrdiff_t i = 0; i < list->get_size(list); ++i) {
        free(array[i]);
    }

    free(array);
}

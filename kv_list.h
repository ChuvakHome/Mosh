
#ifndef _MY_SHELL_KV_LIST_H_
#define _MY_SHELL_KV_LIST_H_

#include <stdbool.h>
#include <stddef.h>

struct pair {
  char* key;
  char* value;
};

void pair_free(struct pair);

struct kv_list;

typedef void (*consumer)(const struct kv_list *node);

struct kv_list {
  struct {
    struct kv_list* head;
    struct kv_list* tail;
  };

  struct pair data;
  struct kv_list *next;

  size_t size;

  void (*add)(struct kv_list *self, struct pair);
  struct kv_list* (*find_node)(struct kv_list *self, const char *key);
  char* (*find)(struct kv_list *self, const char *key);
  void (*put)(struct kv_list *self, const char *key, const char *value);
  void (*foreach)(struct kv_list *self, consumer f);
  char** (*to_array)(struct kv_list *self, bool null_tail);
  size_t (*get_size)(const struct kv_list *self);
};

struct kv_list* kv_list_create();

void kv_list_free(struct kv_list*);

void kv_list_to_array_free(const struct kv_list *list, char **array);

#endif

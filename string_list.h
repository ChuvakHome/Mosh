
#ifndef _MY_SHELL_STRING_LIST_H_
#define _MY_SHELL_STRING_LIST_H_

#include <stdbool.h>
#include <stddef.h>

struct string_list {
  struct {
    struct string_list* head;
    struct string_list* tail;
  };

  char* str;
  size_t size;
  struct string_list *next;

  void (*add)(struct string_list *self, const char *path);
  char** (*to_array)(struct string_list *self, bool null_tail);
  void (*set_string)(struct string_list *self, char *str);
  void (*copy_string)(struct string_list *self, const char *str);
  size_t (*get_size)(const struct string_list *self);
};

struct string_list* string_list_create();

void string_list_free(struct string_list *list);

void string_list_to_array_free(const struct string_list *list, char **array);

#endif


#include "interpreter_utils.h"

#include <string.h>

#include "../global_vars.h"

#include "../utils.h"

struct string_list* split_by_whitespaces(const char *str) {
    struct string_list *list = string_list_create();

    char *pos;

    while ((pos = strtok2(str, " \"'")) != NULL) {
        char symb = pos[0];
        char prev_symb = pos[-1];

        char *new_str;

        if (symb == ' ') {
            new_str = substrp(str, pos);

            if (new_str != NULL)
                list->add(list, new_str);

            free(new_str);

            str = pos + 1;
        }
        else if (prev_symb != '\\') {
              char separ[2] = { symb, 0 };
              char *next_pos;

              char *pos_copy = pos;
              pos++;

              while ((next_pos = strstr((void*) pos, separ)) != NULL) {
                  if (next_pos[0] == symb && next_pos[-1] != '\\')
                      break;

                  pos = next_pos + 1;
              }

              pos = pos_copy;

              if (next_pos != NULL) {
                  new_str = substrp(pos + 1, next_pos);
                  list->add(list, new_str);
                  free(new_str);

                  str = next_pos + 1;
              }
              else
                  break;
         }
    }

    if (str[0] != 0)
        list->add(list, str);

    return list;
}

char* insert_var_values(const char *str) {
    if (strstr(str, "$") == NULL)
        return alloc_and_copy(str);

    char *result = NULL;
    char *pos;

    while ((pos = strstr(str, "$")) != NULL) {
        if (result == NULL)
            result = str != pos ? substrp(str, pos) : alloc_and_copy("");

        char *next_pos = strtok2(pos, " \t\n\r\\");
        char *str_to_add;

        if (next_pos == NULL)
            next_pos = pos + strlen(pos);

        if (next_pos - pos > 1) {
            char *var_name = substrp(pos + 1, next_pos);
            str_to_add = var_list->find(var_list, var_name);

            if (str_to_add == NULL)
                str_to_add = "";

            free(var_name);
        }
        else {
            str_to_add = "$";
        }

        void *addr = result;
        result = concat(result, str_to_add);
        free(addr);

        str = next_pos;
    }

    if (str[0] != 0) {
        void *addr = result;
        result = concat(result, str);
        free(addr);
    }

    return result;
}

#include <stdio.h>

static char* find_any_special_symbol(const char *str) {
    struct kv_list_node *tmp = special_symbols->head;

    while (tmp != NULL) {
        char *pos = strchr(str, tmp->data.key[0]);

        if (pos != NULL)
            return pos;

        tmp = tmp->next;
    }

    return NULL;
}

char* insert_special_symbols(const char *str) {
    char *buf = alloc_and_copy("");
    char *pos;

    while ((pos = find_any_special_symbol(str)) != NULL) {
        char *tmp = substrp(str, pos);
        char spec_sym[] = { pos[0], 0 };

        char *buf_copy = buf;

        buf = concat(buf_copy, tmp);
        free(buf_copy);
        buf_copy = buf;

        buf = concat(buf_copy, special_symbols->find(special_symbols, spec_sym));

        str = pos + 1;

        free(buf_copy);
        free(tmp);
    }

    char *buf_copy = buf;
    buf = concat(buf_copy, str);
    free(buf_copy);

    return buf;
}

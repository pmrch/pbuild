#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "log.h"

DynStrArr* new_str_vec() {
    DynStrArr *base_darr = (DynStrArr*)malloc(sizeof(DynStrArr));
    if (base_darr == NULL) {
        LOG_ERROR("%s", "Failed to allocate memory for dynamic array of file paths!");
        return NULL;
    }

    base_darr->data = (char**)malloc(sizeof(void*) * 2);
    if (base_darr->data == NULL) {
        LOG_ERROR("%s", "Failed to allocate memory for dynamic array data!");
        free(base_darr);
        return NULL;
    }

    base_darr->cap = 2;
    base_darr->len = 0;
    base_darr->freed = false;

    return base_darr;
}

i32 str_vec_push(DynStrArr *restrict arr, const char *elem) {
    if (arr == NULL || arr->freed || arr->data == NULL) {
        LOG_ERROR("%s", "Failed to push str, arr was freed or uninitialized!");
        return -1;
    }

    if (elem == NULL) {
        LOG_ERROR("%s", "Cannot push NULL to string vector!");
        return -1;
    }

    if (arr->len >= arr->cap) {
        // Technically unsafe, practically, will probably never overflow
        usize new_cap = (arr->cap + (arr->cap >> 1));
        char **temp_data = (char**)realloc(arr->data, sizeof(char*) * arr->cap);

        if (temp_data != NULL) {
            arr->data = temp_data; 
            arr->cap = new_cap;
        }
    }

    arr->data[arr->len++] = strdup(elem);
    return 0;
}

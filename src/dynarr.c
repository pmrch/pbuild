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

    base_darr->freed = false;
    base_darr->cap = 8;
    base_darr->len = 0;

    if ((base_darr->data = (char**)malloc(sizeof(void*) * base_darr->cap)) == NULL) {
        LOG_ERROR("%s", "Failed to allocate memory for dynamic array data!");
        free(base_darr);
        return NULL;
    }

    return base_darr;
}

// If elem was heap allocated, you have free it after function call
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
        arr->cap = new_cap;

        LOG_VERBOSE("Realloc called for resizing to <%zu bytes>", sizeof(char*) * arr->cap);
        char **temp_data = (char**)realloc(arr->data, sizeof(char*) * arr->cap);
        if (temp_data != NULL) { arr->data = temp_data; }
    }

    arr->data[arr->len++] = strdup(elem);
    return 0;
}

void free_str_vec(DynStrArr *arr) {
    if (arr == NULL || arr->data == NULL || arr->freed) { return; }
    for (usize i = 0; i < arr->len; i++) { free(arr->data[i]); }
    FREE_ALL(TO_DFREE(arr->data), TO_DFREE(arr));
}

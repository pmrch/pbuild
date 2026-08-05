#ifndef _MSC_VER
#define _POSIX_C_SOURCE 200809L
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "utils.h"
#include "log.h"

#ifdef _MSC_VER
#include <intrin.h>
#include <corecrt.h>
#include <stdbool.h>
#include <immintrin.h>

static void cpuid(i32 leaf, i32 subleaf, i32 out[4]) {
    __cpuidex(out, leaf, subleaf);
}

static CpuFeatures get_supported_isa() {
    CpuFeatures f = {0};
    i32 cpuInfo[4];

    // Get Leaf 1
    cpuid(1, 0, cpuInfo);
    f.sse2 = (cpuInfo[3] & (1 << 26)) != 0;

    bool osxsave = (cpuInfo[2] & (1 << 27)) != 0;
    bool avx     = (cpuInfo[2] & (1 << 28)) != 0;
    if (!(osxsave && avx)) { return f; }

    // OS support check for AVX registers
    usize xcr0 = _xgetbv(0);
    bool ymm_enabled = (xcr0 & 0x6) == 0x6;
    if (!ymm_enabled) { return f; }

    f.avx = true;

    // Get Leaf 7
    cpuid(7, 0, cpuInfo);
    if (cpuInfo[1] & (1 << 5)) { 
        f.avx2 = true; 
    }

    bool zmm_enabled = (xcr0 & 0xE6) == 0xE6;
    if (zmm_enabled && (cpuInfo[1] & (1 << 16))) {
        f.avx512f = true;
    }

    return f;
}

const char* get_best_isa(void) {
    CpuFeatures f = get_supported_isa();

    if (f.avx512f) return "/arch:AVX512";
    if (f.avx2)    return "/arch:AVX2";
    if (f.avx)     return "/arch:AVX";
    if (f.sse2)    return "/arch:SSE2";
    return "";
}
#endif

SplitString *split(const char *str, i32 chr) {
    if (str == NULL) {
        LOG_ERROR("%s", "Cannot split string, provided NULL!");
        return NULL;
    }

    SplitString *ss = (SplitString*)malloc(sizeof(SplitString));
    if (ss == NULL) {
        LOG_ERROR("%s", "Failed to allocate memory for string split destination!");
        return NULL;
    } else {
        ss->strings = (char**)malloc(strlen(str) + 1);
        if (ss->strings == NULL) {
            LOG_ERROR("%s", "Failed to allocate string array within SplitString");
            free(ss);
            return NULL;
        }

        ss->num_split = 0;
    }

    const char *ptr = str;
    while (*ptr != '\0') {
        char *first_member = strchr(ptr, chr);
        if (first_member == NULL) {
            LOG_ERROR("%s", "The string cannot be split, no breakpoint at given character");
            return NULL;
        }

        usize first_member_len = strlen(first_member);
        ss->strings[ss->num_split] = strdup_cross(first_member);
        ptr += (first_member_len + 1);
    }

    return ss;
}

char *strdup_cross(const char *str) {
    #ifdef _MSC_VER
    return _strdup(str);
    
    #else
    return strdup(str);

    #endif
}

char** clone_string_array_mutable(const char **arr, usize num_elem) {
    char **mutable_arr = (char**)malloc((num_elem + 1) * sizeof(char*));
    if (mutable_arr == NULL) {
        LOG_ERROR("%s", "Failed to allocate memory for the copy of the array!");
        return NULL;
    }

    for (usize i = 0; i < num_elem; i++) {
        mutable_arr[i] = strdup_cross(arr[i]);
    }

    mutable_arr[num_elem] = NULL;
    return mutable_arr;
}

FILE* fopen_cross(const char *restrict path, const char *restrict mode) {
    #ifdef _MSC_VER
    FILE *file;
    errno_t result = fopen_s(&file, path, mode);
    return result == 0 ? file : NULL;

    #else
    return fopen(path, mode);
    #endif
}

i32 strcasecmp_cross(const char *restrict s1, const char *restrict s2) {
    #ifdef _MSC_VER
    return _stricmp(s1, s2);

    #else
    return strcasecmp(s1, s2);

    #endif
}

i32 strncasecmp_cross(const char *restrict s1, const char *restrict s2, const usize char_count) {
    #ifdef _MSC_VER
    return _strnicmp(s1, s2, char_count);
    
    #else
    return strncasecmp(s1, s2, char_count);

    #endif
}

i32 strcat_cross(char *restrict dest, usize dest_size, const char *restrict src) {
    if (dest == NULL || src == NULL) {
        LOG_ERROR("%s", "Can't concatenate strings, source or destination was NULL!");
        return -1;
    }

    #ifdef _MSC_VER
    errno_t result = strcat_s(dest, dest_size, src);
    return (i32)result;

    #else
    const usize final_size = strlen(dest) + strlen(src) + 1;
    if (final_size > dest_size) {
        LOG_ERROR("Target buffer was too small for string catenation (%zu bytes < %zu bytes)", dest_size, final_size);
        return -1;
    }
    
    strcat(dest, src);
    return 0;

    #endif
}

i32 create_test_file(void) {
    FILE *f = fopen_cross("test.c", "w");
    if (f != NULL) {
        fprintf(f, "int main(void) { return 0; }\n");
        fclose(f);

        return 0;
    }

    #ifndef _MSC_VER
    if (system("touch test.c && echo \"int main(void) { return 0; }\" > test.c") == 0) {
        return 0;
    }

    #else 
    if (system("echo int main(void) { return 0; } > test.c") == 0) {
        return 0;
    }

    #endif

    return -1;
}

void free_all(const usize count, ...) {
    va_list args;
    va_start(args, count);
    
    for (usize i = 0; i < count; i++) {
        ToFree *obj = va_arg(args, ToFree*);
        if (obj == NULL) { continue; }

        Destructor func = obj->func != NULL ? obj->func : free; 
        func(obj->obj);
    }

    va_end(args);
}

void free_split(SplitString* ss) {
    LOG_DEBUG("%s", "Freeing a SplitString");
    LOG_VERBOSE("Freeing a SplitString pointer at <%p>", (void*)ss);
    if (ss == NULL) { return; }

    if (ss->strings != NULL && ss->num_split > 0) { 
        for (usize i = 0; i < ss->num_split; i++) {
            LOG_VERBOSE("Freeing the %zu. string in SplitString", i);
            free(ss->strings[i]);
        }

        LOG_DEBUG("%s", "Freed all strings from SplitString");
        ss->num_split = 0;
    }

    LOG_VERBOSE("Freeing string array of SplitString at <%p>", (void*)ss->strings);
    free(ss->strings);
    LOG_DEBUG("%s", "Freed string array of SplitString");

    LOG_VERBOSE("Freeing SplitString at <%p>", (void*)ss);
    free(ss);
    LOG_DEBUG("%s", "Freed split string successfully!");
}

// If a sequence of whitespace is found, they get reduced to a singular whitespace
void normalize_whitespaces(char *restrict s) {
    if (s == NULL || *s == '\0') {
        LOG_WARN("%s", "Not normalizing whitespaces, passed NULL or empty string");
        return;
    }

    //LOG_DEBUG("Normalizing whitespaces for string <%s>", s);
    char *read_ptr  = s;
    char *write_ptr = s;
    u8   found_whitespace = 0;

    while (*read_ptr != '\0') {
        if (isspace(*read_ptr)) {
            if (++found_whitespace == 1) {
                *write_ptr++ = ' ';
            }
        } else {
            *write_ptr++ = *read_ptr;
            found_whitespace = 0;
        }

        ++read_ptr;
    }

    *write_ptr = '\0';
}

void to_lowercase(char *restrict str) {
    if (str == NULL || *str == '\0') {
        LOG_WARN("%s", "Cannot convert string to lowercase, NULL or empty string was passed");
        return;
    }

    while (*str != '\0') {
        *str = (char)tolower((i32)*str);
        ++str;
    }
}

void strip_quotes(char *restrict str) {
    if (str == NULL || *str == '\0') {
        LOG_WARN("%s", "Not removing double quotes, passed NULL or empty string");
        return;
    }

    char *read_ptr  = str;
    char *write_ptr = str;

    while (*read_ptr != '\0') {
        if (!(*read_ptr == '"' || *read_ptr == '\'')) {
            *write_ptr++ = *read_ptr;
        }

        ++read_ptr;
    }

    *write_ptr = '\0';
}

void free_mutable_cloned_string_array(char **arr) {
    char **p = arr;
    while (*p != NULL) {
        free(*p++);
    }

    free(arr);
}

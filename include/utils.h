#ifndef UTILS_H
#define UTILS_H

// ===========================================
// =         Shared Included Headers         =
// ===========================================
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// ===========================================
// =      Convenient Type Definitions        =
// ===========================================
typedef uint8_t   u8;
typedef uint32_t u32;

typedef int32_t  i32;
typedef size_t usize;

typedef enum {
    C   = 0,
    Cpp = 1,
} Lang;

typedef enum {
    Debug   = 0,
    Release = 1
} Config;

typedef enum {
    Lazy     = 0,
    Lint     = 1,
    Moderate = 2,
    Strict   = 3
} Strictness;

typedef void (*Destructor)(void *);

typedef struct {
    char **strings;
    usize num_split;
} SplitString;

typedef struct {
    void       *obj;
    Destructor func;
} ToFree;

// ===========================================
// =        Windows CPU intrinsics           =
// ===========================================
#if defined(_MSC_VER)
typedef struct CpuFeatures {
    bool sse2;
    bool avx;
    bool avx2;
    bool avx512f;
} CpuFeatures;

const char* get_best_isa();
#endif

// ===========================================
// =        General Shared Utility           =
// ===========================================

// Splits string, returns array pointer and number of elements
SplitString *split(const char *str, i32 chr);

// Frees all heap-allocated strings of a heap-allocated buffer, 
// and the buffer itself
void free_mutable_cloned_string_array(char **arr);

// If a sequence of whitespace is found, they get reduced to a singular whitespace
void normalize_whitespaces(char *restrict s);

void free_all(const usize count, ...);
void to_lowercase(char *restrict str);
void strip_quotes(char *restrict s);
void free_split(SplitString* ss);

char* strdup_cross(const char *str);
char** clone_string_array_mutable(const char **arr, usize num_elem);

FILE* fopen_cross(const char *restrict path, const char *restrict mode);

int create_test_file();
int strcat_cross(char *restrict dest, size_t dest_size, const char *restrict src);

#endif

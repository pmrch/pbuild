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
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int32_t i32;
typedef int64_t i64;

typedef ptrdiff_t isize;
typedef size_t    usize;

#undef bool
#define bool _Bool

typedef enum {
    C   = 0,
    Cpp = 1,
} Lang;

typedef enum {
    Debug   = 0,
    Release = 1
} Config;

typedef enum {
    Static  = 0,
    Dynamic = 1
} LinkerMode;

typedef enum {
    Lazy     = 0,
    Lint     = 1,
    Moderate = 2,
    Strict   = 3
} Strictness;

typedef void (*Destructor)(void *);

typedef struct {
    char **strings;
    usize  num_split;
} SplitString;

typedef struct {
    void       *obj;
    const char *name;
    Destructor  func;
} ToFree;

typedef struct {
    char **data;
    usize  len;
    usize  cap;
    bool   freed;
} DynStrArr;

// ===========================================
// =        Shared Macro Definitions         =
// ===========================================
#if defined(__clang__)
// ALL clang variants including clang-cl: (void*) cast
#define TO_FREE(obj, func) ((ToFree){ obj, #obj, func != NULL ? (Destructor)(void*)func : free})
#else
// gcc (any libc) + MSVC cl.exe: direct cast
#define TO_FREE(obj, func) ((ToFree){ obj, #obj, func != NULL ? (Destructor)func : free})
#endif

#define TO_DFREE(obj) ((ToFree){ obj, #obj, free })
#define FREE_ALL(...)   \
    free_all_no_vargs(  \
        (ToFree[]){__VA_ARGS__}, \
        sizeof((ToFree[]){__VA_ARGS__}) / sizeof(ToFree)  \
    )

// Fix broken bool detection of compiler
#undef bool
#define bool _Bool

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

const char *get_best_isa(void);
#endif

// ===========================================
// =        General Shared Utility           =
// ===========================================

// clang-format off
// Dynamic array utils
DynStrArr* new_str_vec();
i32 str_vec_push(DynStrArr *restrict arr, const char *elem);

// Returns a newly allocated SplitString.
// The caller owns the returned object and must call free_split().
SplitString* split(const char *str, i32 chr);

char* strdup_cross(const char *str);
char** clone_string_array_mutable(const char **arr, usize num_elem);

FILE* fopen_cross(const char *restrict path, const char *restrict mode);
// clang-format on

// Frees all heap-allocated strings of a heap-allocated buffer,
// and the buffer itself
void free_mutable_cloned_string_array(char **arr);

// If a sequence of whitespace is found, they get reduced to a singular whitespace
void normalize_whitespaces(char *restrict s);
void to_lowercase(char *restrict str);
void strip_quotes(char *restrict s);
void free_split(SplitString *ss);
void cleanup_test();

i32 create_test_file(void);
i32 free_all_no_vargs(ToFree objects[], const usize count);
i32 strcat_cross(char *restrict dest, size_t dest_size, const char *restrict src);

i32 strcasecmp_cross(const char *restrict s1, const char *restrict s2);
i32 strncasecmp_cross(const char *restrict s1, const char *restrict s2, const usize char_count);

bool contains_str(const char **str_arr, const usize num_elem, const char *str);

#endif

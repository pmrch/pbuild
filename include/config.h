#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct {
    const char *cc;
    const char *link;
    char *std;
    char *target;
} CompilerConfig;

typedef struct {
    char *cstd;
    char *cppstd;
} LangStd;

// clang-format off
void free_lang_std(LangStd *std);
void free_compiler_config(CompilerConfig *cfg);

int populate_empty_config(CompilerConfig *cfg);
CompilerConfig* new_config(const char *project_name);
// clang-format on

#endif

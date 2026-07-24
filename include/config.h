#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct {
    const char *cc;
    const char *link;
    char *std;
    char *target;
} CompilerConfig;

int populate_empty_config(CompilerConfig *cfg);
void free_compiler_config(CompilerConfig *cfg);
CompilerConfig* new_config(const char *project_name);

#endif

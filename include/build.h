#ifndef BUILD_H
#define BUILD_H

#include "utils.h"

typedef struct {
    char *command;
    char *cmd_stdout;
    char *cmd_stderr;
} CompileTask;

int compile_code(const char *base_cmd, const char *cwd, const usize num_jobs);

#endif

#ifndef FLAG_HELPERS_H
#define FLAG_HELPERS_H

#include "utils.h"

void clang_clify_flags(char *restrict buf);
void write_std_buf(char *buf, const usize bufsize, const char *std);
void join_ldflags_path(char *restrict mimalloc_path, const usize destsize, const char *path);
void strcat_with_space(char *restrict dest, usize dest_size, const char *restrict src);

#endif

#ifndef PATH_H
#define PATH_H

#include "utils.h"
#include <stdbool.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#undef bool
#define bool _Bool

// clang-format off
// Returned buffer is malloc()'d by the platform's compatible getcwd(),
// so caller must free() it
char* get_cwd(void);

// Returned buffer is strdup()'d, caller must free() it
char* get_basename(char *abs_path);
usize gather_source_files(const char *srcdir);
// clang-format on

bool path_exists(const char *path);
bool is_path_valid(const char *path);

i32  create_directory(const char *path);
void join_path(char *restrict path, const char *restrict child);

#endif

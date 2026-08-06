#ifndef PATH_H
#define PATH_H

#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

#include <stdbool.h>

#undef bool
#define bool _Bool

// Returned buffer is malloc()'d by the platform's compatible getcwd(),
// so caller must free() it
char* get_cwd(void);

// Returned buffer is strdup()'d, caller must free() it
char* get_basename(char *abs_path);

bool is_path_valid(const char *path);

#endif

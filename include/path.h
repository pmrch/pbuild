#ifndef PATH_H
#define PATH_H

#include <stdbool.h>

#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

// Returned buffer is malloc()'d by the platform's compatible getcwd(),
// so caller must free() it
char* get_cwd();

// Returned buffer is strdup()'d, caller must free() it
char* get_basename(char *abs_path);

bool is_path_valid(const char *path);

#endif

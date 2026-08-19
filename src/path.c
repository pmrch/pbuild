// Common imports
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "path.h"
#include "utils.h"

#ifdef _MSC_VER
    #define PATH_SEP "\\"
#else
    #define PATH_SEP "/"
#endif

#undef strchr
#undef strrchr

// Support both Windows and Linux
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <direct.h>
#include <ctype.h>

// Returned buffer is malloc()'d by _getcwd(), so caller must free() it
static char* GetCwdWin(void) {
    char buf[PATH_MAX] = { 0 };
    
    if (GetCurrentDirectory(sizeof(buf), buf) == 0) {
        LOG_ERROR("%s", "Failed to get current working directory!");
        return NULL;
    }

    return strdup_cross(buf);
}

static bool IsPathValidWin(const char *path) {
    if (path == NULL || *path == '\0') {
        LOG_DEBUG("%s", "NULL path was provided!");
        return false;
    }

    bool startsCorrectly = isalpha((int)(path[0])) && path[1] == ':' && path[2] == '\\';
    if (!startsCorrectly) {
        LOG_DEBUG("Path doesn't start correctly <%s>!", path);
        return false;
    }

    const char *ptr = path;
    usize numBackslashes = 0;
    usize dirDepth = 0;

    bool isInDir = false;
    while (*ptr != '\0') {
        if (*ptr == '\\') {
            isInDir = false;
            ++numBackslashes;
        }

        if (*ptr != '\\' && !isInDir) {
            isInDir = true;
            ++dirDepth;
        }

        ++ptr;
    }

    if ((dirDepth > numBackslashes) || dirDepth == 0) {
        return false;
    }

    return true;
}

static i32 CreateDirectoryWin(const char *path) {
    if (CreateDirectory(path, NULL)) { return 0; }

    DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS) { return 0; }
    if (err == ERROR_PATH_NOT_FOUND) { return -1; }

    return -1;
}

static bool PathExistsWin(const char *path) {
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        LOG_DEBUG("Directory at <%s> not found", path);
        return false;
    }

    if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        LOG_WARN("Provided path <%s> was not a directory!", path);
        return false;
    }

    return true;
}

#else
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

// Returned buffer is malloc()'d by getcwd(), so caller must free() it
static char* get_cwd_unix(void) {
    char *buffer = (char*)malloc(PATH_MAX);
    if (buffer == NULL || getcwd(buffer, PATH_MAX) == NULL) {
        LOG_ERROR("%s", "Failed to get current working directory!");
        return NULL;
    }

    return buffer;
}

static bool is_path_valid_unix(const char *path) {
    if (path[0] != '/') { return false; }

    const char *ptr = path;
    usize num_slashes = 0;
    usize dir_depth = 0;

    bool currently_in_dir = false;
    while (*ptr != '\0') {
        if (*ptr == '/') {
            currently_in_dir = false;
            ++num_slashes;
        }

        if (*ptr != '/' && !currently_in_dir) {
            currently_in_dir = true;
            ++dir_depth;
        }

        ++ptr;
    }

    // First condition is technically impossible, but it won't hurt
    if ((dir_depth > num_slashes) || dir_depth == 0) {
        return false;
    }

    return true;
}

static bool path_exists_unix(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) { return false; }

    closedir(dir);
    return true;
}

#endif

// Returned buffer is malloc()'d by the platform's compatible getcwd(),
// so caller must free() it
char* get_cwd(void) {
    #ifdef _MSC_VER
    return GetCwdWin();

    #else
    return get_cwd_unix();

    #endif
}

// Returned buffer is strdup()'d, caller must free() it
char* get_basename(char *abs_path) {
    if (abs_path == NULL) {
        LOG_ERROR("%s", "NULL was provided instead of a valid path when getting filename!");
        return NULL;
    }

    char *last_slash = strrchr(abs_path, (int)PATH_SEP[0]);
    if (last_slash == NULL) {
        last_slash = strrchr(abs_path, '\\'); // MinGW fallback
    }

    if (last_slash == NULL) {
        LOG_WARN("%s", "Returning path instead of filename, failed to extract it!");
    }

    return last_slash ? strdup_cross(++last_slash) : strdup_cross(abs_path);
}

char **gather_source_files(const char *srcdir) {
    const usize num_files = get_num_files(srcdir);
    char **target = (char**)malloc(sizeof(char*) * num_files);

    return target;
}

bool path_exists(const char* path) {
    #ifdef _MSC_VER
    return PathExistsWin(path);

    #else
    return path_exists_unix(path);

    #endif
}

bool is_path_valid(const char *path) {
    if (path == NULL || *path == '\0') {
        LOG_ERROR("%s" , "NULL path or empty string was provided, can't validate");
        return false;
    }

    #ifdef _MSC_VER
    return IsPathValidWin(path);

    #else
    return is_path_valid_unix(path);

    #endif
}

void join_path(char *restrict path, const char *restrict child) {
    if (path == NULL || child == NULL) { 
        LOG_ERROR("%s", "Couldn't join paths, parent and/or child dir was NULL");
        return; 
    }

    strcat_cross(path, PATH_MAX, PATH_SEP);
    strcat_cross(path, PATH_MAX, child);
}

i32 create_directory(const char *path) {
    #ifndef _MSC_VER
    return (mkdir(path, 0755) == 0 || errno == EEXIST) ? 0 : -1;

    #else
    return CreateDirectoryWin(path);

    #endif
}

// Common imports
#include <string.h>

#include "log.h"
#include "path.h"
#include "utils.h"

// Support both Windows and Linux
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <direct.h>
#include <ctype.h>

// Returned buffer is malloc()'d by _getcwd(), so caller must free() it
static char* get_cwd_win() {
    char *buffer = _getcwd(NULL, PATH_MAX);
    if (buffer == NULL) {
        LOG_ERROR("Failed to get current working directory!");
        return NULL;
    }

    return buffer;
}


static bool is_path_valid_win(char *path) {
    bool starts_correctly = !isalpha((int)(path[0])) && path[1] == ':' && path[2] == '\\';
    if (path == NULL || *path == '\0' || !starts_correctly) {
        return false;
    }

    char *ptr = path;
    usize num_backslashes = 0;
    usize dir_depth = 0;

    bool is_in_dir = false;
    while (*ptr != '\0') {
        if (*ptr == '\\') {
            is_in_dir = true;
            ++num_backslashes;
        }

        if (*ptr != '\\' && !is_in_dir) {
            is_in_dir = true;
            ++dir_depth;
        }

        ++ptr;
    }

    if ((dir_depth > num_backslashes) || dir_depth == 0) { 
        return false; 
    }

    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    return true;
}

#else
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

// Returned buffer is malloc()'d by getcwd(), so caller must free() it
static char* get_cwd_unix() {
    char *buffer = (char*)malloc(PATH_MAX);
    if (buffer == NULL || getcwd(buffer, PATH_MAX) == NULL) {
        LOG_ERROR("%s", "Failed to get current working directory!");
        return NULL;
    }

    return buffer;
}

static bool is_path_valid_unix(char *path) {
    if (path[0] != '/') { return false; }

    char *ptr = path;
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

    DIR *dir = opendir(path);
    if (!dir) { return false; }

    closedir(dir);
    return true;
}

#endif

// Returned buffer is malloc()'d by the platform's compatible getcwd(), 
// so caller must free() it
char* get_cwd() {
    #ifdef _MSC_VER
    return get_cwd_win();
    
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

    #ifdef _MSC_VER
        char *last_slash = strrchr(abs_path, '\\');
    #else
        char *last_slash = strrchr(abs_path, '/');
        if (last_slash == NULL) {
            last_slash = strrchr(abs_path, '\\'); // MinGW fallback
        }
    #endif

    if (last_slash == NULL) {
        LOG_WARN("%s", "Returning path instead of filename, failed to extract it!");
    }

    return last_slash ? strdup_cross(++last_slash) : strdup_cross(abs_path);
}

bool is_path_valid(const char *path) {
    if (path == NULL || *path == '\0') {
        LOG_ERROR("%s" , "NULL path or empty string was provided, can't validate");
        return false;
    }

    #ifdef _MSC_VER
    return is_path_valid_win(strdup_cross(path));
    
    #else
    return is_path_valid_unix(strdup_cross(path));

    #endif
}

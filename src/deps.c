#include <stdbool.h>
#include <string.h>

#include "deps.h"
#include "config.h"
#include "parser.h"
#include "log.h"

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#define NTDDI_VERSION 0x0A000000

#include <Windows.h>
#include <stdlib.h>
#include <fileapi.h>

#include "path.h"

static bool is_mimalloc_available_win(const CompilerOptions *opts) {
    if (opts != NULL && opts->mimalloc_lib_path != NULL && *opts->mimalloc_lib_path != '\0') {
        WIN32_FIND_DATAA findData;
        HANDLE dirHandle = FindFirstFileA(opts->mimalloc_lib_path, &findData);

        // Previous step has already confirmed whether it is a valid directory, skipping here
        do {
            bool isRoot = strcmp(findData.cFileName, ".") == 0;
            bool isParent = strcmp(findData.cFileName, "..") == 0;
            if (isRoot || isParent) { continue; }

            char *ext = strrchr(findData.cFileName, '.');
            LOG_VERBOSE("Currently checking %s", findData.cFileName);
            if (ext == NULL) { continue; } else { ++ext; } 

            bool hasMimalloc = strstr(findData.cFileName, "mimalloc") != NULL;
            bool isLibFile = strncmp(ext, "lib", 3) == 0 || strncmp(ext, "dll", 3) == 0;
            
            // If it starts with mimalloc and is a .lib or .dll, return true
            if (hasMimalloc && isLibFile) {
                char buf[PATH_MAX] = { 0 };
                snprintf(buf, sizeof(buf), "%s\\%s", opts->mimalloc_lib_path, findData.cFileName);

                FindClose(dirHandle);
                return true; 
            }
        } while (FindNextFile(dirHandle, &findData) != 0);

        FindClose(dirHandle);
    }

    i32 res = system("vcpkg --version > NUL 2>&1");
    if (res != 0) { LOG_DEBUG("%s", "vcpkg was not found on PATH"); }

    char vcpkg_root[PATH_MAX] = { 0 };
    DWORD vcpkg_root_res = GetEnvironmentVariable("VCPKG_ROOT", vcpkg_root, sizeof(vcpkg_root));
    if (vcpkg_root_res == ERROR_ENVVAR_NOT_FOUND) {
        LOG_WARN("%s", "VCPKG_ROOT wasn't found in environment variables");
        return false;
    }

    return true;
}

#else
#include <dirent.h>
#include <errno.h>

static bool is_mimalloc_available_unix(const CompilerOptions *opts) {
    if (opts == NULL || opts->mimalloc_lib_path == NULL || *opts->mimalloc_lib_path == '\0') {
        LOG_ERROR("%s", "");
        return false;
    }

    DIR *dir = opendir(opts->mimalloc_lib_path);
    if (dir == NULL) {
        LOG_ERROR("Failed to open mimalloc library directory at <%s>!", opts->mimalloc_lib_path);
        return false;
    }

    struct dirent *entry = NULL;
    errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            LOG_DEBUG("Skipping reading <%s>", entry->d_name);
            continue;
        }

        char *base = strchr(entry->d_name, '.');
        if (base == NULL) {
            //LOG_DEBUG("Directory <%s/%s> had no basename or extension", opts->mimalloc_lib_path, entry->d_name);
            continue;
        }

        bool has_mimalloc = strstr(entry->d_name, "mimalloc") != NULL;
        (void)has_mimalloc;
    }

    if (entry == NULL && errno != 0) {
        LOG_ERROR("An error occured reading the directory <%s>", opts->mimalloc_lib_path);
        closedir(dir);
        return false;
    }

    closedir(dir);

    /*snprintf(cmd, sizeof(cmd), "%s -lmimalloc test.c -o test > /dev/null 2>&1", cfg.cc);

    if (create_test_file() != 0) { return false; }
    if (system(cmd) == 0) { return true; }*/

    return true;
}

#endif

bool is_mimalloc_available(const CompilerConfig cfg, const CompilerOptions *opts) {
    LOG_DEBUG("%s", "Entered is_mimalloc_available");
    (void)cfg;

    #ifdef _MSC_VER
    return is_mimalloc_available_win(opts);

    #else
    return is_mimalloc_available_unix(opts);

    #endif
}

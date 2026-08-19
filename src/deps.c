#ifndef _MSC_VER
    #define _POSIX_C_SOURCE 200809L
#endif

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "deps.h"
#include "flags.h"
#include "utils.h"
#include "config.h"
#include "parser.h"
#include "log.h"

#undef strchr
#undef strrchr
#undef strstr

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdlib.h>
#include <fileapi.h>

#include "path.h"

static bool is_mimalloc_available_win(const CompilerOptions opts) {
    if (opts.mimalloc_lib_path != NULL && *opts.mimalloc_lib_path != '\0') {
        WIN32_FIND_DATAA findData;
        HANDLE dirHandle = FindFirstFileA(opts.mimalloc_lib_path, &findData);

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
                snprintf(buf, sizeof(buf), "%s\\%s", opts.mimalloc_lib_path, findData.cFileName);

                FindClose(dirHandle);
                return true;
            }
        } while (FindNextFile(dirHandle, &findData) != 0);

        FindClose(dirHandle);
    }

    return false;
}

// clang-format off
static bool is_system_mimalloc_available_win(const CompilerOptions opts, const CompilerConfig cfg, char *restrict flag, const usize flag_size) {
// clang-format on
    char *compiler = get_compiler(opts, cfg);
    if (compiler == NULL) {
        LOG_ERROR("%s", "Can't detect system mimalloc due to missing compiler!");
        return false;
    }

    char vcpkg_root[PATH_MAX] = { 0 };
    DWORD vcpkg_root_res = GetEnvironmentVariable("VCPKG_ROOT", vcpkg_root, sizeof(vcpkg_root));
    if (vcpkg_root_res == ERROR_ENVVAR_NOT_FOUND) {
        LOG_WARN("%s", "VCPKG_ROOT wasn't found in environment variables");
        return false;
    }

    i32 res = system("vcpkg --version > NUL 2>&1");
    if (res != 0) { LOG_WARN("%s", "vcpkg was not found on PATH"); return false; }

    system("vcpkg --version");
    return false;
}

#else
#include <dirent.h>
#include <errno.h>

static bool is_mimalloc_available_unix(const CompilerOptions opts) {
    if (opts.mimalloc_lib_path == NULL || *opts.mimalloc_lib_path == '\0') {
        LOG_ERROR("%s", "Cannot check the presence of mimalloc, lib path provided was NULL!");
        return false;
    }

    DIR *dir = opendir(opts.mimalloc_lib_path);
    if (dir == NULL) {
        LOG_ERROR("Failed to open mimalloc library directory at <%s>!", opts.mimalloc_lib_path);
        return false;
    }

    struct dirent *entry = NULL;
    errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            LOG_DEBUG("Skipping reading <%s>", entry->d_name);
            continue;
        }

        const char *base = strchr(entry->d_name, '.');
        if (base == NULL) {
            //LOG_DEBUG("Directory <%s/%s> had no basename or extension", opts->mimalloc_lib_path, entry->d_name);
            continue;
        }

        bool has_mimalloc = strstr(entry->d_name, "mimalloc") != NULL;
        bool is_lib = ``;
        

        if (has_mimalloc && is_lib) {
            closedir(dir);
            return true;
        }
    }

    if (entry == NULL && errno != 0) {
        LOG_ERROR("An error occured reading the directory <%s>", opts.mimalloc_lib_path);
        closedir(dir);
        return false;
    }

    closedir(dir);
    return true;
}

static bool is_system_mimalloc_available_unix(const CompilerOptions opts, const CompilerConfig cfg, char *restrict flag, const usize flag_size) {
    char *compiler = get_compiler(opts, cfg);

    if (compiler == NULL) {
        LOG_ERROR("%s", "Can't detect system mimalloc due to missing compiler!");
        return false;
    }

    bool return_code = false;
    bool pkgconf_works = system("pkg-config --version > /dev/null 2>&1") == 0;
    if (!pkgconf_works) { return false; }

    FILE *stream = NULL;
    char buffer[64] = { 0 };
    char str_buf[256] = { 0 };

    stream = popen("pkg-config --libs --cflags mimalloc 2>/dev/null", "r");
    if (stream != NULL) {
        while (fgets(buffer, sizeof(buffer), stream) != NULL) {
            if (str_buf[0] == 0) {
                snprintf(str_buf, sizeof(str_buf), "%s", buffer);
                str_buf[strlen(str_buf) - 1] = '\0';
                LOG_DEBUG("Read the following from pkg-config: %s", str_buf);

                return_code = true;
                break;
            }
        }

        pclose(stream);
    }

    if (return_code) { snprintf(flag, flag_size, "%s", str_buf);}
    return return_code;
}

#endif

bool is_mimalloc_available(const CompilerOptions opts) {
    LOG_DEBUG("%s", "Entered is_mimalloc_available");

    #ifdef _MSC_VER
    return is_mimalloc_available_win(opts);

    #else
    return is_mimalloc_available_unix(opts);

    #endif
}

bool is_system_mimalloc_available(const CompilerOptions opts, const CompilerConfig cfg, char *restrict flag, const usize flag_size) {
    #ifndef _MSC_VER
    return is_system_mimalloc_available_unix(opts, cfg, flag, flag_size);

    #else
    (void)flag;
    (void)flag_size;
    
    return is_system_mimalloc_available_win(opts, cfg);

    #endif
}

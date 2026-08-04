#include <Windows.h>
#include <fileapi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "deps.h"
#include "config.h"
#include "parser.h"
#include "utils.h"
#include "path.h"
#include "log.h"

#ifdef _MSC_VER
static bool is_mimalloc_available_win(const CompilerOptions *opts) {
    WIN32_FIND_DATAA findData;
    HANDLE dirHandle = FindFirstFileA(opts->mimalloc_lib_path, &findData);

    // Previous step has already confirmed whether it is a valid directory, skipping here
    do {
        bool isRoot = strcmp(findData.cFileName, ".") == 0;
        bool isParent = strcmp(findData.cFileName, "..") == 0;
        if (isRoot || isParent) { continue; }

        char *ext = strrchr(findData.cFileName, '.');
        if (ext == NULL) {
            LOG_ERROR("%s", "There was not file in mimalloc lib directory that can be linked");
            return false;
        } else { ++ext; } 

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

    i32 res = system("vcpkg --version");
    if (res != 0) { LOG_DEBUG("%s", "vcpkg was not found on PATH"); }

    char vcpkg_root[PATH_MAX] = { 0 };
    DWORD vcpkg_root_res = GetEnvironmentVariable("VCPKG_ROOT", vcpkg_root, sizeof(vcpkg_root));
    if (vcpkg_root_res == ERROR_ENVVAR_NOT_FOUND) {
        LOG_WARN("%s", "VCPKG_ROOT wasn't found in environment variables");
        return false;
    }

    return true;
}
#endif

bool is_mimalloc_available(const CompilerConfig cfg, const CompilerOptions *opts) {
    char cmd[512] = { 0 };

    #ifdef _MSC_VER
    return is_mimalloc_available_win(opts);

    #else
    snprintf(cmd, sizeof(cmd), "%s -lmimalloc test.c -o test > /dev/null 2>&1", cfg.cc);

    if (create_test_file() != 0) { return false; }
    if (system(cmd) == 0) { return true; }

    return true;

    #endif
}

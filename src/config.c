#define MAX_VERSION 23

#include <stdlib.h>
#include <stdbool.h>

#include "config.h"
#include "utils.h"
#include "path.h"
#include "log.h"

#ifndef _MSC_VER
#include <stdio.h>
#include <unistd.h>

static void cleanup_test() {
    remove("test");
    remove("test.c");
    remove("test.exe");
}

static char* get_latest_std(const char *cc) {
    if (cc == NULL) { return NULL; }
    if (create_test_file() != 0) {
        return strdup_cross("c99");
    }

    unsigned int ref_size = sizeof("clang -std=c23 -c test.c -o test > /dev/null 2>&1");
    unsigned int version = MAX_VERSION;

    char cmd[512];
    char buf[4];

    while (version >= 11 && version <= MAX_VERSION) {
        #ifdef _WIN32
        snprintf(cmd, ref_size, "%s -std=c%u -c test.c -o test.exe > NUL 2>&1", cc, version);
        #else
        snprintf(cmd, ref_size, "%s -std=c%u -c test.c -o test > /dev/null 2>&1", cc, version);
        #endif

        snprintf(buf, sizeof(buf), "c%u", version);
        int res = system(cmd);
        
        if (res == 0) {
            cleanup_test();
            return strdup_cross(buf);
        }

        version -= 6;
    }

    cleanup_test();

    #ifdef _WIN32
    snprintf(cmd, ref_size, "%s -std=c99 -c test.c -o test.exe > NUL 2>&1", cc);
    #else
    snprintf(cmd, ref_size, "%s -std=c99 -c test.c -o test > /dev/null 2>&1", cc);
    #endif

    return system(cmd) == 0 ? strdup_cross("c99") : NULL;
}
#endif

CompilerConfig* new_config(const char *project_name) {
    CompilerConfig *cfg_base = malloc(sizeof(CompilerConfig));
    if (cfg_base == NULL) {
        LOG_ERROR("Failed to allocate memory for compiler config");
        return NULL;
    }

    if (project_name == NULL) {
        char *path = get_cwd();
        cfg_base->target = get_basename(path);
    } else { 
        cfg_base->target = strdup_cross(project_name); 
    }

    #if defined(_WIN32) || defined(_WIN64)
    cfg_base->std = "clatest";
    cfg_base->cc = "cl.exe";
    cfg_base->link = "link.exe";

    #else
    i32 gnu_version_works = system("gcc --version >/dev/null 2>&1") == 0 && system("g++ --version >/dev/null 2>&1") == 0;
    i32 clang_version_works = system("clang --version >/dev/null 2>&1") == 0 && system("clang++ --version >/dev/null 2>&1") == 0;

    const char *compiler = gnu_version_works == 0 ? "gcc" 
        : clang_version_works ? "clang" : NULL;

    char *std = get_latest_std(compiler);
    cfg_base->std = std != NULL ? std : "c99";

    cfg_base->cc = compiler;
    cfg_base->link = compiler;
    #endif

    return cfg_base;
}

void free_compiler_config(CompilerConfig *cfg) {
    if (cfg->target != NULL) { free(cfg->target); }
    free(cfg);
}
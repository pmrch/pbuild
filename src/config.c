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

static const char *detect_compiler(void) {
    #if defined(_MSC_VER) && defined(__clang__)
    return "clang-cl.exe";

    #elif defined(_MSC_VER) && !defined(__clang__)
    return "cl.exe";

    #elif defined(__clang__)
    return "clang";

    #elif defined(__GNUC__)
    return "gcc";

    #else
    i32 gnu_version_works = system("gcc --version >/dev/null 2>&1") == 0 && system("g++ --version >/dev/null 2>&1") == 0;
    i32 clang_version_works = system("clang --version >/dev/null 2>&1") == 0 && system("clang++ --version >/dev/null 2>&1") == 0;

    return gnu_version_works ? "gcc" : clang_version_works ? "clang" : NULL;
    #endif
}

CompilerConfig* new_config(const char *project_name) {
    LOG_DEBUG("%s", "initializing CompilerConfig");
    CompilerConfig *cfg_base = (CompilerConfig*)malloc(sizeof(CompilerConfig));
    LOG_VERBOSE("Initializing CompilerConfig at <%p>", (void*)cfg_base);

    if (cfg_base == NULL) {
        LOG_ERROR("%s", "Failed to allocate memory for compiler config");
        return NULL;
    }

    if (project_name == NULL) {
        char *path = get_cwd();
        cfg_base->target = get_basename(path);
    } else { 
        cfg_base->target = strdup_cross(project_name); 
    }

    // Get compiler
    const char *compiler = detect_compiler();

    #ifdef _MSC_VER
    cfg_base->cc = compiler;
    cfg_base->std = "clatest";
    cfg_base->link = "link.exe";

    LOG_DEBUG("%s", "Detected platform: Windows");
    return cfg_base;

    #else
    char *std = get_latest_std(compiler);
    cfg_base->std = std != NULL ? std : strdup_cross("c99");
    LOG_DEBUG("Detected latest language standard <%s>", cfg_base->std);

    #endif

    LOG_DEBUG("Assigned <%s> to config's target", cfg_base->target);
    LOG_DEBUG("%s", "Detected platform: Linux");

    cfg_base->cc = compiler;
    cfg_base->link = compiler;

    LOG_VERBOSE("Returning base CompilerConfig with address <%p>", (void*)cfg_base);
    return cfg_base;
}

void free_compiler_config(CompilerConfig *cfg) {
    LOG_DEBUG("Freeing CompilerConfig at <%p>", (void*)cfg);
    if (cfg->target != NULL) { 
        LOG_VERBOSE("Freeing compiler target at: <%p>", (void*)cfg->target);
        free(cfg->target); 
        LOG_VERBOSE("%s", "Freed target");
    }

    LOG_VERBOSE("Freeing language standard string at <%p>", (const void*)(cfg->std));
    free(cfg->std);

    free(cfg);
    LOG_DEBUG("%s", "Freed CompilerConfig");
}

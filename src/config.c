#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "config.h"
#include "parser.h"
#include "utils.h"
#include "path.h"
#include "log.h"

#ifndef _MSC_VER

#include <stdio.h>
#include <unistd.h>

static void write_test_command_c(char *restrict cmd, const usize ref_size, const Compiler compilers, const u32 version) {
    #ifdef _WIN32 
        snprintf(cmd, ref_size, "%s -std=c%u -c test.c -o test.exe > NUL 2>&1", compilers.cc, version);
    #else
        snprintf(cmd, ref_size, "%s -std=c%u -c test.c -o test.exe > /dev/null 2>&1", compilers.cc, version); 
    #endif
}

static void write_test_command_cpp(char *restrict cmd, const usize ref_size, const Compiler compilers, const u32 version) {
    #ifdef _WIN32 
        snprintf(cmd, ref_size, "%s -std=c++%u -c test.cpp -o test.exe > NUL 2>&1", compilers.cxx, version);
    #else
        snprintf(cmd, ref_size, "%s -std=c++%u -c test.cpp -o test.exe > /dev/null 2>&1", compilers.cxx, version); 
    #endif
}

static void write_default_ver_command(char *restrict cmd, const usize ref_size, const Compiler compilers) {
    if (compilers.cpp_first) {
        #ifndef _WIN32
            snprintf(cmd, ref_size, "%s -std=c++11 -c test.cpp -o test > /dev/null 2>&1", compilers.cxx); 
        #else
            snprintf(cmd, ref_size, "%s -std=c++11 -c test.cpp -o test.exe > NUL 2>&1", compilers.cxx); 
        #endif
    } else {
        #ifndef _WIN32
            snprintf(cmd, ref_size, "%s -std=c++11 -c test.c -o test > /dev/null 2>&1", compilers.cc); 
        #else
            snprintf(cmd, ref_size, "%s -std=c++11 -c test.c -o test.exe > NUL 2>&1", compilers.cc);
        #endif
    }
}

static LangStd* get_latest_std(const Compiler compilers) {
    if (compilers.cc == NULL || compilers.cxx == NULL) { return NULL; }
    u32 MAX_VERSION = 23;
    u32 step = 6;

    if (compilers.cpp_first) {
        MAX_VERSION = 26;
        step = 3;
    }
    
    LangStd *stds = (LangStd*)malloc(sizeof(LangStd));
    if (stds == NULL) {
        LOG_ERROR("%s", "Failed to allocate memory for LangStd struct!");
        return NULL;
    }

    if (create_test_file() != 0) {
        stds->cppstd = strdup_cross("c++11");
        stds->cstd = strdup_cross("c99");
        return stds;
    }

    u32 version = MAX_VERSION;
    char cmd[512];
    char buf[8];

    while (version >= 11 && version <= MAX_VERSION) {
        if (compilers.cpp_first) { write_test_command_cpp(cmd, sizeof(cmd), compilers, version); } 
        else { write_test_command_c(cmd, sizeof(cmd), compilers, version); }

        if (compilers.cpp_first) { snprintf(buf, sizeof(buf), "c%u", version); } 
        else { snprintf(buf, sizeof(buf), "c++%u", version); }

        int res = system(cmd);
        if (res == 0) {
            cleanup_test();
            
            if (compilers.cpp_first) { stds->cppstd = strdup_cross(buf); }
            else { stds->cstd = strdup_cross(buf); }            
            return stds;
        }

        version -= step;
    }

    write_default_ver_command(cmd, sizeof(cmd), compilers);
    cleanup_test();

    return system(cmd) == 0 ? stds : NULL;
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

void free_lang_std(LangStd *std) {
    if (std == NULL) { return; }

    // Fre children
    free(std->cppstd);
    free(std->cstd);

    // Free parent
    free(std);
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
        free(path);
    } else { 
        cfg_base->target = strdup_cross(project_name); 
    }

    LOG_DEBUG("Assigned <%s> to config's target", cfg_base->target);
    const char *compiler = detect_compiler();

    #ifdef _MSC_VER
    cfg_base->cc = compiler;
    cfg_base->std = strdup_cross("clatest");

    #ifdef __clang__
    cfg_base->link = "clang-cl.exe"

    #else
    cfg_base->link = "link.exe";

    #endif

    LOG_DEBUG("%s", "Detected platform: Windows");
    LOG_VERBOSE("Returning base CompilerConfig with address <%p>", (void*)cfg_base);
    return cfg_base;

    #else
    Compiler compilers = pair_compiler(compiler);
    LangStd *std = get_latest_std(compilers);


    if (std == NULL) {
        LOG_ERROR("%s", "Failed to determine latest language standard!");
        free_compiler_config(cfg_base);
        return NULL;
    }

    char *final_std = NULL;
    if (compilers.cpp_first) { final_std = strdup_cross(std->cppstd); }
    else { final_std = strdup_cross(std->cstd); }

    fprintf(stderr, "Assigned std to: <%p>\n", (void*)std);

    free_lang_std(std);
    cfg_base->std = final_std != NULL ? strdup_cross(final_std) : strdup_cross("c99");

    LOG_DEBUG("Detected latest language standard <%s>", cfg_base->std);
    LOG_DEBUG("%s", "Detected platform: Linux");

    cfg_base->cc = compiler;
    cfg_base->link = compiler;

    LOG_VERBOSE("Returning base CompilerConfig with address <%p>", (void*)cfg_base);
    return cfg_base;

    #endif
}

void free_compiler_config(CompilerConfig *cfg) {
    if (cfg == NULL) {
        LOG_WARN("%s", "Not freeing CompilerConfig, was NULL!");
        return;
    }

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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "config.h"
#include "parser.h"
#include "flags.h"
#include "utils.h"
#include "deps.h"
#include "path.h"
#include "log.h"

Strictness validate_strictness(const char* level_str) {
    if (level_str == NULL || *level_str == '\0') {
        LOG_ERROR("%s", "Couldn't validate strictness as NULL or empty value was passed! Defaulting to Moderate linting");
        return Moderate;
    }

    char *end = NULL;

    errno = 0;
    long level = strtol(level_str, &end, 10);
    int is_integer = (errno != ERANGE && *end == '\0');

    if (is_integer) {
        if (level == 3) { return Strict; }
        else if (level == 2) { return Moderate; }
        else if (level == 1) { return Lint; }
        else if (level == 0) { return Lazy; }
    } 
    
    LOG_WARN("Invalid strictness level '%s' was set, defaulting to moderate", level_str);
    return Moderate;
}

const char* delegate_strictness_flags(const Strictness level) {
    if (level == Strict) { return STRICT_FLAGS; }
    if (level == Moderate) { return MODERATE_FLAGS; }
    if (level == Lint) { return LINT_FLAGS; }
    
    return " ";
}

static void strcat_with_space(char *restrict dest, usize dest_size, const char *restrict src) {
    strcat_cross(dest, dest_size, " ");
    strcat_cross(dest, dest_size, src);
}

static char* get_compiler(const CompilerOptions opts, const CompilerConfig cfg) {
    char *compiler = NULL;
    
    if (opts.compiler_set && opts.compiler.cc != NULL && opts.compiler.cxx != NULL) {
        if (opts.lang_set) {
            if (opts.lang == Cpp) { compiler = strdup_cross(opts.compiler.cxx); } 
            else { compiler = strdup_cross(opts.compiler.cc); }
        } else { 
            compiler = strdup_cross(opts.compiler.cc); 
        }
    } else { 
        compiler = strdup_cross(cfg.cc); 
    }

    return compiler;
}

static void write_std_buf(char *buf, const usize bufsize, const char *std) {
    #ifdef _MSC_VER
    snprintf(buf, bufsize, "/std:%s", std);

    #else
    snprintf(buf, bufsize, "-std=%s", std);

    #endif
}

static void join_ldflags_path(char *mimalloc_path, const usize destsize, const char *path) {
    #ifdef _MSC_VER
    snprintf(mimalloc_path, destsize, "/LIBPATH:%s mimalloc-static.lib", path);

    #else
    snprintf(mimalloc_path, destsize, "-L%s -lmimalloc", path);

    #endif
}

char* join_cflags(const CompilerOptions opts, const CompilerConfig cfg) {
    LOG_INFO("%s", "Running join_cflags");
    char cflags[PATH_MAX] = { 0 };
    const usize dest_size = sizeof(cflags);

    char *compiler = get_compiler(opts, cfg);
    if (compiler == NULL) {
        LOG_ERROR("%s", "No compiler has been defined anywhere");
        return NULL;
    } else { LOG_DEBUG("Got compiler <%s>", compiler); }

    snprintf(cflags, sizeof(cflags), "%s", compiler);
    free(compiler);

    if (cfg.std != NULL && *cfg.std != '\0') {
        char std_buf[15] = { 0 };
        write_std_buf(std_buf, sizeof(std_buf), cfg.std);
        strcat_with_space(cflags, dest_size, std_buf);
    }

    strcat_with_space(cflags, dest_size, CFLAGS_BASE);
    if (opts.config_set && opts.config == Debug) {
        strcat_with_space(cflags, dest_size, DEBUG_FLAGS_CC);
    } else {
        strcat_with_space(cflags, dest_size, OPTIMIZATION_FLAGS);
    }

    if (opts.strictness_set) {
        const char *strictness = delegate_strictness_flags(opts.strictness);
        strcat_with_space(cflags, dest_size, strictness);
    } else {
        strcat_with_space(cflags, dest_size, delegate_strictness_flags(Moderate));
    }

    return strdup_cross(cflags);
}

char* construct_ldflags(const CompilerOptions *opts, const CompilerConfig cfg) {
    LOG_INFO("%s", "Running construct_ldflags");
    char ldflags[PATH_MAX] = { 0 };
    const usize dest_size = sizeof(ldflags);

    if (cfg.link == NULL || *cfg.link == '\0') {
        LOG_ERROR("%s", "There was no linker specified!");
        return NULL;
    }

    snprintf(ldflags, dest_size, "%s %s", cfg.link, LINKER_FLAGS);
    //LOG_DEBUG("Wrote the following to ldflags: %s", ldflags);
    if (opts->mimalloc_lib_path != NULL && *opts->mimalloc_lib_path != '\0') {
        if (!is_mimalloc_available(cfg, opts)) { 
            LOG_DEBUG("%s", "Mimalloc was not available, trying to download...");
            return NULL; 
        }

        char mimalloc_path[PATH_MAX] = { 0 };
        join_ldflags_path(mimalloc_path, sizeof(mimalloc_path), opts->mimalloc_lib_path);
        strcat_with_space(ldflags, dest_size, mimalloc_path);
    }

    return strdup_cross(ldflags);
}

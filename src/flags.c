#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "flaghelpers.h"
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

static void populate_temp_cflags(char *restrict dest, const ToolchainFlags tcflags, const usize dest_size, const CompilerOptions opts) {
    if (opts.config_set && opts.config == Debug) {
        strcat_with_space(dest, dest_size, tcflags.debug);

        if (opts.linker_mode_set && opts.linker_mode == Static) {
            if (tcflags.type == COMPILER_CLANGCL || tcflags.type == COMPILER_CL) {
                strcat_with_space(dest, dest_size, "/MTd");
            }
        }
    } else {
        strcat_with_space(dest, dest_size, tcflags.release);

        if (opts.linker_mode_set && opts.linker_mode == Static) {
            if (tcflags.type == COMPILER_CLANGCL) { strcat_with_space(dest, dest_size, "/clang:-static"); }
            if (tcflags.type == COMPILER_CL) { strcat_with_space(dest, dest_size, "/MT"); }
        }
    }

    if (opts.strictness_set) {
        const char *strictness = delegate_strictness_flags(opts.strictness, tcflags.type);
        strcat_with_space(dest, dest_size, strictness);
    } else {
        LOG_DEBUG("%s", "Strictness was not set! Defaulting to moderate");
        strcat_with_space(dest, dest_size, delegate_strictness_flags(Moderate, tcflags.type));
    }


    if (opts.linker_mode_set && opts.linker_mode == Static) {
        if (tcflags.type == COMPILER_CLANGCL) { strcat_with_space(dest, dest_size, "/clang:-static"); }
        if (tcflags.type == COMPILER_CL) { strcat_with_space(dest, dest_size, "/MT"); }
    }
}

static const ToolchainFlags FLAG_MATRIX[] = {
    { COMPILER_GCC,      CFLAGS_BASE_UNIX, DEBUG_FLAGS_CC_UNIX,      OPTIMIZATION_FLAGS_UNIX     },
    { COMPILER_CLANG,    CFLAGS_BASE_UNIX, DEBUG_FLAGS_CC_UNIX,      OPTIMIZATION_FLAGS_UNIX     },
    { COMPILER_CL,       CFLAGS_BASE_CL,   DEBUG_FLAGS_CC_CL,        OPTIMIZATION_FLAGS_CL       },
    { COMPILER_CLANGCL,  CFLAGS_BASE_CL,   DEBUG_FLAGS_CC_CLANG_CL,  OPTIMIZATION_FLAGS_CLANG_CL },
};

static char *get_cflags(const CompilerType ctype, const CompilerOptions opts) {
    LOG_DEBUG("Got CompilerType <%u>", ctype);
    if (ctype == UNKNOWN) { return NULL; }

    char cflags_temp[PATH_MAX / 2] = { 0 };
    usize matrix_elems = sizeof(FLAG_MATRIX) / sizeof(ToolchainFlags);

    if (ctype == COMPILER_GCC || ctype == COMPILER_CLANG) {
        snprintf(cflags_temp, sizeof(cflags_temp), "%s", CFLAGS_BASE_UNIX);
    } else if (ctype == COMPILER_CLANGCL || ctype == COMPILER_CL) {
        snprintf(cflags_temp, sizeof(cflags_temp), "%s", CFLAGS_BASE_CL);
    }

    for (usize i = 0; i < matrix_elems; i++) {
        if (FLAG_MATRIX[i].type == ctype) {
            populate_temp_cflags(cflags_temp, FLAG_MATRIX[i], sizeof(cflags_temp), opts);
            LOG_DEBUG("Loop at %zuth index with ctype being <%u>", i, FLAG_MATRIX[i].type);
            return strdup_cross(cflags_temp);
        }
    }

    return NULL;
}

static const char *get_ldflags(const CompilerType ctype, const CompilerOptions opts) {
    if (ctype == UNKNOWN) { return NULL; }

    if (opts.config_set && opts.config == Debug) {
        if (ctype == COMPILER_GCC || ctype == COMPILER_CLANG) { return DEBUG_FLAGS_LNK_UNIX; }
        if (ctype == COMPILER_CL) { return DEBUG_FLAGS_LNK_CL; }
        if (ctype == COMPILER_CLANGCL) { return DEBUG_FLAGS_LNK_CLANG_CL; }
    } else {
        if (ctype == COMPILER_GCC || ctype == COMPILER_CLANG) { return LINKER_FLAGS_UNIX; }
        if (ctype == COMPILER_CL) { return LINKER_FLAGS_CL; }
        if (ctype == COMPILER_CLANGCL) { return LINKER_FLAGS_CLANG_CL; }
    }

    return NULL;
}

static CompilerType get_type(const char *compiler) {
    if (strcmp(compiler, "gcc") == 0 || strcmp(compiler, "g++") == 0) { return COMPILER_GCC; }
    if (strcmp(compiler, "clang") == 0 || strcmp(compiler, "clang++") == 0) { return COMPILER_CLANG; }
    if (strcmp(compiler, "cl.exe") == 0) { return COMPILER_CL; }
    if (strcmp(compiler, "clang-cl.exe") == 0) { return COMPILER_CLANGCL; }

    return UNKNOWN;
}

const char* delegate_strictness_flags(const Strictness level, const CompilerType ctype) {
    if (level == Strict) {
        LOG_DEBUG("Entered branch level == Strict, ctype currently is <%u>", ctype);

        if (ctype == COMPILER_CLANG) { return (STRICT_FLAGS_UNIX " " CLANG_ONLY_STRICT_FLAGS " " CLANG_UNREASONABLE_DISABLED_FLAGS); }
        if (ctype == COMPILER_GCC) { return (STRICT_FLAGS_UNIX " " GCC_ONLY_STRICT_FLAGS); }
        if (ctype == COMPILER_CL) { return STRICT_FLAGS_CL; }
        if (ctype == COMPILER_CLANGCL) {} // TODO: Add whitespace normalization, convert to /clang: style flags
    }

    if (level == Moderate) {
        if (ctype == COMPILER_CLANG || ctype == COMPILER_GCC) { return MODERATE_FLAGS_UNIX; }
        if (ctype == COMPILER_CL) { return MODERATE_FLAGS_CL; }
        if (ctype == COMPILER_CLANGCL) {}  // TODO: Refer to flags.c line 91 comment
    }

    if (level == Lint) {
        if (ctype == COMPILER_GCC || ctype == COMPILER_CLANG) { return LINT_FLAGS_UNIX; }
        if (ctype == COMPILER_CL) { return LINT_FLAGS_CL; }
        if (ctype == COMPILER_CLANGCL) { return ("/clang:" LINT_FLAGS_UNIX); }
    }

    return " ";
}

char* get_compiler(const CompilerOptions opts, const CompilerConfig cfg) {
    char *compiler = NULL;

    if (opts.compiler_set && opts.compiler.cc != NULL && opts.compiler.cxx != NULL) {
        if (opts.lang_set) {
            if (opts.lang == Cpp) { compiler = strdup_cross(opts.compiler.cxx); }
            else { compiler = strdup_cross(opts.compiler.cc); }
        } else {
            if (opts.compiler.cpp_first) { compiler = strdup_cross(opts.compiler.cxx); }
            else { compiler = strdup_cross(opts.compiler.cc); }
        }
    } else {
        compiler = strdup_cross(cfg.cc);
    }

    return compiler;
}

static const char* get_linker(char *compiler) {
    if (compiler == NULL) {
        LOG_ERROR("%s", "Couldn't determine linker from compiler, compiler was NULL!");
        return NULL;
    }

    bool gnu = strcmp(compiler, "gcc") == 0 || strcmp(compiler, "g++") == 0;
    bool clang = strcmp(compiler, "clang") == 0 || strcmp(compiler, "clang++") == 0;
    if (gnu || clang) { return compiler; }

    if (strcmp(compiler, "clang-cl") == 0 || strcmp(compiler, "clang-cl.exe") == 0) { return "clang-cl.exe"; }
    if (strcmp(compiler, "cl") == 0 || strcmp(compiler, "cl.exe") == 0) { return "link.exe"; }

    LOG_ERROR("Couldn't determine linker from compiler <%s>", compiler);
    return NULL;
}



Cflags* join_cflags(const CompilerOptions opts, const CompilerConfig cfg) {
    LOG_INFO("%s", "Running join_cflags");

    Cflags *cflags_final = (Cflags*)malloc(sizeof(Cflags));
    if (cflags_final == NULL) {
        LOG_ERROR("%s", "Failed to allocate memory for cflags");
        return NULL;
    }

    char cflags[PATH_MAX] = { 0 };
    const usize dest_size = sizeof(cflags);

    char *compiler = get_compiler(opts, cfg);
    if (compiler == NULL) {
        LOG_ERROR("%s", "No compiler has been defined anywhere");
        free_cflags(cflags_final);
        return NULL;
    }

    LOG_DEBUG("Got compiler <%s>", compiler);
    CompilerType compiler_type = get_type(compiler);
    snprintf(cflags, sizeof(cflags), "%s", compiler);
    LOG_DEBUG("Derived compiler type <%u>", compiler_type);

    if (cfg.std != NULL && *cfg.std != '\0') {
        char std_buf[15] = { 0 };
        write_std_buf(std_buf, sizeof(std_buf), cfg.std);
        strcat_with_space(cflags, dest_size, std_buf);
    }

    char *cflags_combined = get_cflags(compiler_type, opts);
    if (cflags_combined != NULL) {
        strcat_with_space(cflags, dest_size, cflags_combined);
        free(cflags_combined);
    } else { LOG_ERROR("%s", "Couldn't derive cflags, skipping!"); }

    cflags_final->compiler = compiler;
    cflags_final->flags = strdup_cross(cflags);
    return cflags_final;
}

// clang-format off
char* construct_ldflags(const CompilerOptions opts, const CompilerConfig cfg, char *compiler) {
// clang-format on
    LOG_INFO("%s", "Running construct_ldflags");
    char ldflags[PATH_MAX] = { 0 };
    const usize dest_size = sizeof(ldflags);

    const char *linker = get_linker(compiler);
    if (linker == NULL || *linker == '\0') {
        if (cfg.link != NULL && *cfg.link != '\0') { linker = cfg.link; }
        else { return NULL; }
    }

    CompilerType ctype = get_type(compiler);
    const char* ldflags_temp = get_ldflags(ctype, opts);
    if (ldflags_temp == NULL) {
        LOG_ERROR("%s", "Failed to derive ldflags!");
        return NULL;
    }

    snprintf(ldflags, dest_size, "%s %s", linker, ldflags_temp);
    if (opts.mimalloc_lib_path != NULL && *opts.mimalloc_lib_path != '\0') {
        if (!is_mimalloc_available(opts)) {
            LOG_DEBUG("%s", "Mimalloc was not available, trying to download...");
            return NULL;
        }

        char mimalloc_path[PATH_MAX] = { 0 };
        join_ldflags_path(mimalloc_path, sizeof(mimalloc_path), opts.mimalloc_lib_path);
        strcat_with_space(ldflags, dest_size, mimalloc_path);
    }

    if (opts.use_system_mimalloc) {
        char mimalloc_ldflag[256] = { 0 };

        if(is_system_mimalloc_available(opts, cfg, mimalloc_ldflag, sizeof(mimalloc_ldflag))) {
            strcat_with_space(ldflags, dest_size, mimalloc_ldflag);
        } else {
            LOG_WARN("%s", "Failed to detect system mimalloc");
        }
    }

    #ifndef _MSC_VER
    if (opts.linker_mode_set && opts.linker_mode == Static) {
        strcat_with_space(ldflags, dest_size, "-static");
    }
    #endif

    return strdup_cross(ldflags);
}

void free_cflags(Cflags *cflags) {
    if (cflags == NULL) { return; }

    // Free children
    free(cflags->compiler);
    free(cflags->flags);

    // Free parent
    free(cflags);
}

#include <stdlib.h>
#include <string.h>

#ifndef _MSC_VER
#include <dirent.h>
#endif

#include "parser.h"
#include "flags.h"
#include "utils.h"
#include "path.h"
#include "log.h"

Compiler pair_compiler(const char *compiler) {
    if (compiler == NULL || *compiler == '\0') {
        LOG_ERROR("%s", "Compiler value was parsed as NULL, cannot pair!");
        return INVALID_COMPILER;
    }

    if (strcmp(compiler, "gcc") == 0) { return (Compiler){ .cc="gcc", .cxx="g++" }; }
    if (strcmp(compiler, "clang") == 0) { return (Compiler){ .cc="clang", .cxx="clang++" }; }
    if (strcmp(compiler, "cl") == 0) { return (Compiler){ .cc="cl.exe", .cxx="cl.exe" }; }

    if (strcmp(compiler, "clang++") == 0) { return (Compiler){ .cc="clang", .cxx="clang++" }; }
    if (strcmp(compiler, "g++") == 0) { return (Compiler){ .cc="gcc", .cxx="g++" }; }

    LOG_WARN("%s", "Compiler was not defined! Returning NULL values!");
    return INVALID_COMPILER;
}

static bool is_compiler_valid(const char *str) {
    if (str == NULL || *str == '\0') {
        LOG_ERROR("%s", "Cannot validate compiler as NULL value was passed!");
        return false;
    }

    const char* const valid_compilers[] = { "gcc", "clang", "clang-cl", "cl", "g++", "clang++" };
    for (usize i = 0; i < sizeof(valid_compilers) / sizeof(valid_compilers[0]); i++) {
        if (strncmp(str, valid_compilers[i], strlen(valid_compilers[i])) == 0) {
            return true;
        }
    }

    LOG_ERROR("Invalid compiler provided: <%s>", str);
    return false;
}

static void set_strictness(char *restrict strictness, CompilerOptions *opts) {
    if (opts == NULL) {
        LOG_ERROR("%s", "Couldn't set strictness level, opts was NULL!");
        return;
    }

    if (strictness == NULL || *strictness == '\0') {
        LOG_ERROR("%s", "Couldn't set strictness level, NULL was passed!");
        return;
    }

    opts->strictness = validate_strictness(strictness);
    opts->strictness_set = true;
}

static void set_config(char *restrict config, CompilerOptions *opts) {
    if (opts == NULL) {
        LOG_ERROR("%s", "Cannot set build type, since opts was NULL!");
        return;
    }

    if (config == NULL || *config == '\0') {
        LOG_ERROR("%s", "Cannot set build type, since config was NULL!");
        return;
    }

    if (strncmp(config, "release", 7) == 0) { opts->config = Release; }
    else if (strncmp(config, "debug", 5) == 0) { opts->config = Debug; }
    else {
        LOG_WARN("Invalid config value '%s' provided, defaulting to optimized release build", config);
        opts->config = Release;
    }

    opts->config_set = true;
}

static void set_compiler(char *restrict compiler, CompilerOptions *opts) {
    if (opts == NULL) {
        LOG_ERROR("%s", "Cannot set build type, since opts was NULL!");
        return;
    }

    if (!is_compiler_valid(compiler)) {
        LOG_WARN("Invalid compiler '%s' provided, using system default", compiler);

        #if defined(_MSC_VER) && defined(__clang__)
        opts->compiler = (Compiler){ .cc="clang-cl.exe", .cxx="clang-cl.exe" };

        #elif defined(_MSC_VER) && !defined(__clang__)
        opts->compiler = (Compiler){ .cc="cl.exe", .cxx="cl.exe" }

        #elif defined(__clang__)
        opts->compiler = (Compiler){ .cc="clang", .cxx="clang++" };

        #elif defined(__GNUC__)
        opts->compiler = (Compiler){ .cc="gcc", .cxx="g++" };

        #endif
    } else {
        opts->compiler = pair_compiler(compiler);
    }

    opts->compiler_set = true;
}

static void set_linker_mode(char *restrict mode, CompilerOptions *opts) {
    if (opts == NULL) {
        LOG_ERROR("%s", "Cannot set linker mode, since opts was NULL!");
        return;
    }

    if (mode == NULL || *mode == '\0') {
        LOG_ERROR("%s", "Cannot set linker mode, NULL or empty string was passed!");
        return;
    }

    if (strcmp(mode, "static") == 0) { opts->linker_mode = Static; }
    else if (strcmp(mode, "dynamic") == 0) { opts->linker_mode = Dynamic; }
    else {
        LOG_WARN("Provided linking mode <%s> was invalid, defaulting to static if possible, otherwise dynamic", mode);
        opts->linker_mode = Static;
    }

    printf("Got the following --linker-mode setting: <%s>, and setting is <%u>\n", mode, opts->linker_mode);
    opts->linker_mode_set = true;
}

static void set_lang(char *restrict lang, CompilerOptions *opts) {
    if (opts == NULL) {
        LOG_ERROR("%s", "Failed to set language due to opts being NULL!");
        return;
    }

    if (lang == NULL || *lang == '\0' || strlen(lang) > 3) {
        opts->lang_set = false;
        return;
    }

    bool is_c = strcmp(lang, "c") == 0;
    bool is_cpp = strncmp(lang, "c++", 3) == 0
        || strncmp(lang, "cpp", 3) == 0
        || strncmp(lang, "cxx", 3) == 0;

    if (is_cpp) { opts->lang = Cpp; }
    else if (is_c) { opts->lang = C; }
    else {
        LOG_WARN("Invalid --lang value <%s>, defaulting to C", lang);
        opts->lang = C;
    }

    opts->lang_set = true;
}

// Expects a path that contains the dynamically or statically linkable
// mimalloc library .a/.so/.lib file(s)
static void set_mimalloc(char *restrict path, CompilerOptions *opts) {
    if (opts == NULL) {
        LOG_ERROR("%s", "Failed to set mimalloc, since opts was NULL!");
        return;
    }

    if (opts->use_system_mimalloc) {
        LOG_ERROR("%s", "--with-system-mimalloc and --with-mimalloc are incompatible, only one of them can be specified, or just simply neither");
        free(path);
        return;
    }

    LOG_DEBUG("Setting mimalloc up with path <%s>", path);
    if (!is_path_valid(path)) {
        LOG_WARN("Provided path <%s> was invalid! Can't look for mimalloc.", path);
        opts->use_system_mimalloc = true;
        return;
    }

    opts->use_system_mimalloc = false;
    opts->mimalloc_lib_path = path;
}

CompilerOptions* parse_compiler_flags(const int argc, const char **argv) {
    CompilerOptions *opts = (CompilerOptions*)calloc(1, sizeof(CompilerOptions));
    if (opts == NULL) {
        LOG_ERROR("%s", "Failed to allocate memory for CompilerOptions!");
        return NULL;
    }

    char **argv_clone = clone_string_array_mutable(argv, (usize)argc);
    if (argv_clone == NULL) {
        FREE_ALL(TO_DFREE(opts->mimalloc_lib_path), TO_DFREE(opts));
        return NULL;
    }

    // Create a pointer to travel through the origin pointer
    char **argv_p = argv_clone;

    // Pre-increment to skip first argument which is the executable name itself
    while (*++argv_p != NULL) {
        strip_quotes(*argv_p);
        char *value = strrchr(*argv_p, '=');

        if (strncmp(*argv_p, "--with-system-mimalloc", 22) == 0 && !opts->use_system_mimalloc) {
            opts->use_system_mimalloc = true;
            continue;
        }

        if (value == NULL || *value == '\0') { continue; }
        char *original_casing = strdup_cross(++value);
        to_lowercase(value);

        if (strncmp(*argv_p, "--with-mimalloc=", 16) == 0 && opts->mimalloc_lib_path == NULL) {
            LOG_DEBUG("Received mimalloc path <%s>", original_casing);
            set_mimalloc(original_casing, opts);
        }

        if (strncmp(*argv_p, "--linker-mode=", 14) == 0 && !opts->linker_mode_set) {
            set_linker_mode(value, opts);
        }

        if (strncmp(*argv_p, "--strictness=", 13) == 0 && !opts->strictness_set) {
            set_strictness(value, opts);
        }

        if (strncmp(*argv_p, "--compiler=", 11) == 0 && !opts->compiler_set) {
            set_compiler(value, opts);
        }

        if (strncmp(*argv_p, "--config=", 9) == 0 && !opts->config_set) {
            set_config(value, opts);
        }

        if (strncmp(*argv_p, "--lang=", 7) == 0 && !opts->lang_set) {
            set_lang(value, opts);
        }
    }

    free_mutable_cloned_string_array(argv_clone);
    LOG_INFO("We got %d args", argc);
    return opts;
}

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "parser.h"
#include "build.h"
#include "flags.h"
#include "utils.h"
#include "path.h"
#include "log.h"

#ifndef _MSC_VER
    #include <unistd.h>
    static isize get_num_cpu() { return sysconf(_SC_NPROCESSORS_ONLN); }
#else
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    static usize get_num_cpu() { SYSTEM_INFO si; GetSystemInfo(&si); return si.dwNumberOfProcessors; }
#endif

static i32 verify_arguments(const char *restrict const *argv, const i32 argc) {
    LOG_DEBUG("Verifying command line arguments, found %d args", argc - 1);
    if (argc == 1) { return 0; }

    usize buf_size = (usize)argc * sizeof(argv[0]);
    const char **invalid_flags = (const char**)calloc(1, buf_size);
    LOG_VERBOSE("Allocated %zu bytes for invalid flags at %p", buf_size, (void*)invalid_flags);

    if (invalid_flags == NULL) {
        LOG_ERROR("%s", "Failed to allocate memory for");
        return -1;
    }

    i32 invalid_counter = 0;
    const char **invalid_flags_p = invalid_flags;

    for (i32 i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (!(arg[0] == '-' && arg[1] == '-')) {
            *invalid_flags_p++ = arg;
            invalid_counter++;
        }
    }

    if (invalid_flags != NULL && invalid_flags[0] != NULL && *invalid_flags[0] != '\0') {
        LOG_DEBUG("Found invalid argument <%s>", invalid_flags[0]);

        for (i32 i = 0; i < invalid_counter; i++) {
            LOG_WARN("Unknown option: '%s'", invalid_flags[i]);
        }
    }

    LOG_VERBOSE("Freeing invalid flags buffer at <%p>", (void*)invalid_flags);
    free(invalid_flags);

    LOG_DEBUG("%s", "Freed internal buffers for argument validation");
    LOG_DEBUG("verify_arguments return code: %d", invalid_counter);
    return invalid_counter;
}

i32 main(i32 argc, const char **argv) {
    fprintf(stderr, "Current log level: %d\n", LOG_LEVEL);
    i32 ver_args = verify_arguments(argv, argc);

    if (ver_args != 0) {
        LOG_ERROR("%d invalid argument(s) have been provided!", ver_args);
        return -1;
    }

    CompilerConfig *cfg = new_config("compile_project");
    if (cfg == NULL) { return -1; }

    LOG_DEBUG("Created new config with: \nCompilerConfig {\n\tcc: %s\n\tlink: %s\n\tstd: %s\n\ttarget: %s\n}",
        cfg->cc, cfg->link, cfg->std, cfg->target
    );

    char *cwd = get_cwd();
    LOG_DEBUG("Got current working directory <%s>", cwd);
    LOG_VERBOSE("Current working directory is at: <%p>", (void*)cwd);

    CompilerOptions *opts = parse_compiler_flags(argc, argv);
    if (opts == NULL || cfg == NULL) {
        LOG_ERROR("%s", "Failed to setup options and default config!");
        return(FREE_ALL(TO_FREE(cfg, free_compiler_config), TO_DFREE(cwd), TO_DFREE(opts->mimalloc_lib_path), TO_DFREE(opts)));
    }

    Cflags *compilation_flags = join_cflags(*opts, *cfg);
    normalize_whitespaces(compilation_flags->flags);
    //fprintf(stderr, "Final compilation command: %s\n", compilation_flags->flags);
    compile_code(compilation_flags->flags, cwd, get_num_cpu());

    char *linker_flags = construct_ldflags(*opts, *cfg, compilation_flags->compiler);
    normalize_whitespaces(linker_flags);
    LOG_INFO("Final linker command: %s", linker_flags);

    FREE_ALL(
        TO_DFREE(linker_flags), TO_FREE(compilation_flags, free_cflags), TO_DFREE(opts->mimalloc_lib_path), TO_DFREE(opts),
        TO_DFREE(cwd), TO_FREE(cfg, free_compiler_config),
    );

    return 0;
}

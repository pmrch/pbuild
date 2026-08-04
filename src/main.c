#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "path.h"
#include "flags.h"
#include "utils.h"
#include "config.h"
#include "parser.h"

#define TO_FREE(obj, func) (&(ToFree){ obj, func != NULL ? (Destructor)(void*)func : free})
#define TO_FREE_DEF(obj) (&(ToFree){ obj, free })

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
        LOG_DEBUG("First invalid flag is %s", invalid_flags[0]);
    }

    LOG_VERBOSE("Freeing invalid flags buffer at <%p>", (void*)invalid_flags);
    free(invalid_flags);

    LOG_DEBUG("%s", "Freed internal buffers for argument validation");
    LOG_DEBUG("verify_arguments return code: %d", invalid_counter);
    return invalid_counter;
}

i32 main(i32 argc, const char **argv) {
    fprintf(stderr, "Current log level: %d\n", LOG_LEVEL);
    CompilerConfig *cfg = new_config("compile_project");
    if (cfg == NULL) { return -1; }

    LOG_DEBUG("Created new config with: \nCompilerConfig {\n\tcc: %s\n\tlink: %s\n\tstd: %s\n\ttarget: %s\n}", 
        cfg->cc, cfg->link, cfg->std, cfg->target
    );

    char *cwd = get_cwd();
    LOG_DEBUG("Got current working directory <%s>", cwd);
    LOG_VERBOSE("Current working directory is at: <%p>", (void*)cwd);

    if (verify_arguments(argv, argc) != 0) { goto cleanup; }
    CompilerOptions *opts = parse_compiler_flags(argc, argv);

    if (opts == NULL || cfg == NULL) {
        LOG_ERROR("%s", "Failed to setup options!");
        goto cleanup;
    }

    char *compilation_flags = join_cflags(*opts, *cfg);
    normalize_whitespaces(compilation_flags);
    fprintf(stderr, "Final compilation command: %s\n", compilation_flags);

    char *linker_flags = construct_ldflags(opts, *cfg);
    normalize_whitespaces(linker_flags);
    fprintf(stderr, "Final linker command: %s\n", linker_flags);

    LOG_VERBOSE("Freeing compilation flags at: <%p>", (void*)compilation_flags);
    free(compilation_flags);

    LOG_VERBOSE("Freeing linker flags at: <%p>", (void*)linker_flags);
    free(linker_flags);

    LOG_VERBOSE("Freeing CompilerOptions at: <%p>", (void*)opts);
    free(opts->mimalloc_lib_path);
    free(opts);

    goto cleanup;
    cleanup:
        free_all(2, TO_FREE(cfg, free_compiler_config), TO_FREE_DEF(cwd));
        return 0;
}

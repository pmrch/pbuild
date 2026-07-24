#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "path.h"
#include "build.h"
#include "utils.h"
#include "config.h"
#include "parser.h"


static i32 verify_arguments(const char *restrict const *argv, const i32 argc) {
    const char **invalid_flags = malloc((usize)argc * sizeof(argv[0]));
    if (invalid_flags == NULL) {
        LOG_ERROR("Failed to allocate memory for ");
        return -1;
    }

    i32 invalid_counter = 0;
    for (i32 i = 0; i < argc; i++) {
        const char *arg = argv[i];
        
        if (!(arg[0] == '-' && arg[0] == '-')) {
            *invalid_flags++ = arg;
            ++invalid_counter;
        }
    }

    free(invalid_flags);
    return invalid_counter;
}

i32 main(i32 argc, const char **argv) {
    CompilerConfig *cfg = new_config("compile_project");
    if (cfg == NULL) { return -1; }

    char *cwd = get_cwd();
    if (cwd == NULL) { goto cleanup; return -1; }
    if (verify_arguments(argv, argc) != 0) { goto cleanup; }

    ya();
    CompilerOptions *opts = parse_compiler_flags(argc, argv);
    printf("Set lang: %d | Set compiler: %d | Set config: %d | Set trictness: %d\n", 
        opts->lang_set, opts->compiler_set, opts->config_set, opts->strictness_set
    );

    goto cleanup;
    cleanup:
        free_compiler_config(cfg);
        free(cwd);
        return 0;
}
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "utils.h"
#include "deps.h"

bool is_mimalloc_available(CompilerConfig *cfg, char *restrict out) {
    char cmd[512] = { 0 };
    
    #ifndef _MSC_VER
    usize strsize = sizeof("clang -lmimalloc test.c -o test > /dev/null 2>&1");
    snprintf(cmd, strsize, "%s -lmimalloc test.c -o test > /dev/null 2>&1", cfg->cc);

    if (create_test_file() != 0) { return false; }
    if (system(cmd) == 0) { return true; }

    // Discard unused output
    (void)(out);

    #else
    /// TODO: Write Windows code for finding vcpkg, derive lib paths and try linking with mimalloc

    #endif

    return false;
}

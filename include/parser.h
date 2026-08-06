#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "utils.h"

#define INVALID_COMPILER ((Compiler){ .cc=NULL, .cxx=NULL })

typedef struct {
    const char *cc;
    const char *cxx;
} Compiler;

typedef struct {
    Compiler    compiler;
    char        *mimalloc_lib_path; // Library search path for mimalloc (-L or /LIBPATH:)
    Lang        lang;  
    Config      config;
    Strictness  strictness;   
    bool        lang_set;
    bool        config_set;
    bool        compiler_set;
    bool        strictness_set;
    bool        use_system_mimalloc;
} CompilerOptions;

CompilerOptions* parse_compiler_flags(const int argc, const char **argv);
Compiler pair_compiler(const char *compiler);

#endif

#ifndef FLAGS_H
#define FLAGS_H

#include "config.h"
#include "parser.h"
#include "utils.h"

// ======================================================
// =          Specialized compiler/lang flags           =
// ======================================================
#define C_ONLY_MODERATE_FLAGS_UNIX "-Wstrict-prototypes"
#define C_ONLY_FLAGS_UNIX "-Wmissing-prototypes -Wold-style-definition -Wwrite-strings"
#define GCC_ONLY_STRICT_FLAGS "-Wmaybe-uninitialized -Wduplicated-cond -Wduplicated-branches -Wlogical-op"
#define CLANG_ONLY_STRICT_FLAGS "-Weverything"
#define CLANG_UNREASONABLE_DISABLED_FLAGS "-Wno-unsafe-buffer-usage -Wno-disabled-macro-expansion -Wno-unknown-warning-option"

// ======================================================
// =        General Clang/GCC strictness flags          =
// ======================================================
#define CFLAGS_BASE_UNIX "-Iinclude -MMD -MP"
#define DEBUG_FLAGS_CC_UNIX "-O0 -g -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined"
#define DEBUG_FLAGS_LNK_UNIX "-fsanitize=address -fsanitize=undefined"

#define OPTIMIZATION_FLAGS_UNIX "-flto -ffast-math -O3 -march=native"
#define LINKER_FLAGS_UNIX "-flto"

#define LINT_FLAGS_UNIX "-Wall"

#define MODERATE_FLAGS_UNIX LINT_FLAGS_UNIX " " \
    "-Wextra -Wpedantic -Werror -Wuninitialized -Wconversion -Wsign-conversion -Wcast-align -Wstrict-aliasing=2 " \
    "-Wswitch-enum -Wredundant-decls -Wshadow -Wundef -Wformat=2"

#define STRICT_FLAGS_UNIX MODERATE_FLAGS_UNIX " " \
    "-Wcast-qual -Wpointer-arith -Warray-bounds -Wnull-dereference -Wvla -Wformat-security -Wwrite-strings "\
    "-Wdouble-promotion -Wfloat-equal -Wswitch-default -Wunused -Wunused-function -Wunused-variable -Wunused-parameter " \
    "-Wno-padded -Wno-declaration-after-statement -Wno-jump-misses-init"

// ======================================================
// =                MSVC-specific flags                 =
// ======================================================
#define CFLAGS_BASE_CL "/nologo /LTCG"

#define OPTIMIZATION_FLAGS_CL "/O2 /Oi /Ot /GL /Gy /fp:fast"
#define LINKER_FLAGS_CL "/LTCG /OPT:REF /OPT:ICF"

#define DEBUG_FLAGS_CC_CL "/Od /Zi /RTC1 /fsanitize:address"
#define DEBUG_FLAGS_LNK_CL "/DEBUG /fsanitize:address"

#define OPTIMIZATION_FLAGS_CLANG_CL "/Gy /fp:fast /clang:-march=native /clang:-ffast-math /clang:-O3"
#define LINKER_FLAGS_CLANG_CL "/OPT:REF /OPT:ICF"

#define DEBUG_FLAGS_CC_CLANG_CL "/Od /Zi /Oy- -fsanitize=address,undefined"
#define DEBUG_FLAGS_LNK_CLANG_CL "/DEBUG -fsanitize=address,undefined"

#define LINT_FLAGS_CL "/W3"
#define MODERATE_FLAGS_CL "/W4 /permissive-"
#define STRICT_FLAGS_CL MODERATE_FLAGS_CL " " \
    "/w14456 /w14457 /w14668 /w14061 /w14062 /w14244 /w14242 /w14018"

typedef enum {
    COMPILER_GCC     = (unsigned int)0,
    COMPILER_CLANG   = (unsigned int)1,
    COMPILER_CLANGCL = (unsigned int)2,
    COMPILER_CL      = (unsigned int)3,
    UNKNOWN          = (unsigned int)4
} CompilerType;

typedef struct {
    char *flags;
    char *compiler;
} Cflags;

typedef struct {
    CompilerType type;
    const char  *base;
    const char  *debug;
    const char  *release;
} ToolchainFlags;

// clang-format off
Strictness validate_strictness(const char *level_str);
Cflags* join_cflags(const CompilerOptions opts, const CompilerConfig cfg);

const char* delegate_strictness_flags(const Strictness level, const CompilerType ctype);
char* construct_ldflags(const CompilerOptions opts, const CompilerConfig cfg, char *compiler);
char* get_compiler(const CompilerOptions opts, const CompilerConfig cfg);
// clang-format on

void free_cflags(Cflags *cflags);

#endif

#include "config.h"
#include "parser.h"
#include "utils.h"

#ifdef __clang__
    #define STRICT_FLAGS "-Wall -Wextra -Wpedantic -Werror -Wuninitialized -Wold-style-definition          \
        -Wsign-conversion -Wcast-align -Wcast-qual -Wstrict-aliasing=2 -Wpointer-arith -Warray-bounds      \
        -Wnull-dereference -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wredundant-decls -Wvla   \
        -Wshadow -Wundef -Wformat=2 -Wformat-security -Wwrite-strings -Wdouble-promotion -Wfloat-equal     \
        -Wswitch-enum -Wswitch-default -Wunused -Wunused-function -Wunused-variable -Wunused-parameter     \
        -Wlogical-op -Wno-padded -Wno-declaration-after-statement -Weverything -Wno-unknown-warning-option \
        -Wno-disabled-macro-expansion"

    #define STRICT_FLAGS_CPP "-Wall -Wextra -Wpedantic -Werror -Wuninitialized -Wconversion -Wcast-align    \
        -Wcast-qual -Wstrict-aliasing=2 -Wpointer-arith -Warray-bounds -Wnull-dereference -Wno-padded       \
        -Wredundant-decls -Wvla -Wshadow -Wundef -Wformat=2 -Wformat-security -Wwrite-strings -Wfloat-equal \
        -Wswitch-enum -Wswitch-default -Wunused -Wunused-function -Wunused-variable -Wunused-parameter      \
        -Wlogical-op -Weverything -Wdouble-promotion -Wno-declaration-after-statement -Wsign-conversion     \
        -Wno-unknown-warning-option -Wno-disabled-macro-expansion"
#elif defined(__GNUC__)
    #define STRICT_FLAGS "-Wall -Wextra -Wpedantic -Werror -Wuninitialized -Wmaybe-uninitialized       \
        -Wconversion -Wsign-conversion -Wcast-align -Wcast-qual -Wstrict-aliasing=2 -Wpointer-arith    \
        -Warray-bounds -Wnull-dereference -Wmissing-prototypes -Wstrict-prototypes -Wwrite-strings     \
        -Wredundant-decls -Wshadow -Wundef -Wformat=2 -Wformat-security  -Wvla -Wold-style-definition  \
        -Wdouble-promotion -Wfloat-equal -Wswitch-enum -Wswitch-default -Wunused -Wunused-function     \
        -Wunused-variable -Wunused-parameter -Wduplicated-cond -Wduplicated-branches -Wlogical-op      \
        -Wno-padded -Wno-declaration-after-statement"

    #define STRICT_FLAGS_CPP "-Wall -Wextra -Wpedantic -Werror -Wuninitialized -Wmaybe-uninitialized     \
        -Wconversion -Wsign-conversion -Wcast-align -Wcast-qual -Wstrict-aliasing=2 -Wpointer-arith      \
        -Warray-bounds -Wnull-dereference -Wwrite-strings -Wredundant-decls -Wshadow -Wundef -Wformat=2  \
        -Wformat-security -Wvla -Wdouble-promotion -Wfloat-equal -Wswitch-enum -Wswitch-default -Wunused \
        -Wunused-function -Wunused-variable -Wunused-parameter -Wduplicated-cond -Wduplicated-branches   \
        -Wlogical-op -Wno-padded -Wno-declaration-after-statement"
#else
    #define STRICT_FLAGS "/W4 /permissive- /w14456 /w14457 /w14668 /w14061 /w14062 /w14244 /w14242 /w14018"
    #define MODERATE_FLAGS "/W4 /permissive-"
    #define LINT_FLAGS "/W3"
#endif

#if defined(__clang__) || defined(__GNUC__)
    #define LINT_FLAGS "-Wall"
    #define MODERATE_FLAGS "-Wall -Wextra -Wpedantic -Werror -Wuninitialized -Wconversion     \
        -Wsign-conversion -Wcast-align -Wstrict-aliasing=2 -Wmissing-prototypes -Wswitch-enum \
        -Wredundant-decls -Wshadow -Wundef -Wformat=2 -Wwrite-strings -Wstrict-prototypes"

    #define MODERATE_FLAGS_CPP "-Wall -Wextra -Wpedantic -Werror -Wuninitialized -Wconversion \
        -Wsign-conversion -Wcast-align -Wstrict-aliasing=2 -Wswitch-enum -Wredundant-decls    \
        -Wshadow -Wundef -Wformat=2 -Wwrite-strings"

    #define CFLAGS_BASE "-Iinclude -MMD -MP"

    #define OPTIMIZATION_FLAGS "-flto -ffast-math -O3 -march=native"
    #define LINKER_FLAGS "-flto"

    #define DEBUG_FLAGS_CC "-O0 -g -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined"
    #define DEBUG_FLAGS_LNK "-fsanitize=address -fsanitize=undefined"
#else
    #define CFLAGS_BASE "/nologo /LTCG"

    #define OPTIMIZATION_FLAGS "/O2 /Oi /Ot /GL /Gy /fp:fast"
    #define LINKER_FLAGS "/LTCG /OPT:REF /OPT:ICF"

    #define DEBUG_FLAGS_CC "/Od /Zi /RTC1 /fsanitize:address"
    #define DEBUG_FLAGS_LNK "/DEBUG /fsanitize:address"
#endif

Strictness validate_strictness(const char *level_str);
const char* delegate_strictness_flags(const Strictness level);
char* join_cflags(const CompilerOptions opts, const CompilerConfig cfg);

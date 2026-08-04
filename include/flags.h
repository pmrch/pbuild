#include "config.h"
#include "parser.h"
#include "utils.h"

#if defined(_MSC_VER) && !defined(__clang__)
    #define STRICT_FLAGS "/W4 /permissive- /w14456 /w14457 /w14668 /w14061 /w14062 /w14244 /w14242 /w14018"
    #define MODERATE_FLAGS "/W4 /permissive-"
    #define LINT_FLAGS "/W3"

#elif defined(_MSC_VER) && defined(__clang__)
    #define STRICT_FLAGS "/clang:-Wall /clang:-Wextra /clang:-Wpedantic /clang:-Werror /clang:-Wuninitialized /clang:-Wold-style-definition \
        /clang:-Wsign-conversion /clang:-Wcast-align /clang:-Wcast-qual /clang:-Wstrict-aliasing=2 /clang:-Wpointer-arith /clang:-Warray-bounds \
        /clang:-Wnull-dereference /clang:-Wmissing-prototypes /clang:-Wstrict-prototypes /clang:-Wconversion /clang:-Wredundant-decls /clang:-Wvla \
        /clang:-Wshadow /clang:-Wundef /clang:-Wformat=2 /clang:-Wformat-security /clang:-Wwrite-strings /clang:-Wdouble-promotion /clang:-Wfloat-equal \
        /clang:-Wswitch-enum /clang:-Wswitch-default /clang:-Wunused /clang:-Wunused-function /clang:-Wunused-variable /clang:-Wunused-parameter \
        /clang:-Wno-padded /clang:-Wno-declaration-after-statement /clang:-Weverything /clang:-Wno-jump-misses-init /clang:-Wno-unsafe-buffer-usage \
        /clang:-Wno-disabled-macro-expansion /clang:-Wno-unknown-warning-option"

    #define STRICT_FLAGS_CPP "/clang:-Wall /clang:-Wextra /clang:-Wpedantic /clang:-Werror /clang:-Wuninitialized /clang:-Wsign-conversion /clang:-Wcast-align \
        /clang:-Wcast-qual /clang:-Wstrict-aliasing=2 /clang:-Wpointer-arith /clang:-Warray-bounds /clang:-Wnull-dereference /clang:-Wdouble-promotion \
        /clang:-Wredundant-decls /clang:-Wvla /clang:-Wshadow /clang:-Wundef /clang:-Wformat=2 /clang:-Wformat-security /clang:-Wwrite-strings /clang:-Wconversion \
        /clang:-Wfloat-equal /clang:-Wswitch-enum /clang:-Wswitch-default /clang:-Wunused /clang:-Wunused-function /clang:-Wunused-variable /clang:-Wunused-parameter \
        /clang:-Wno-padded /clang:-Weverything /clang:-Wno-jump-misses-init /clang:-Wno-unsafe-buffer-usage /clang:-Wno-disabled-macro-expansion \
        /clang:-Wno-unknown-warning-option"

#elif defined(__clang__) && !defined(_MSC_VER)
    #define STRICT_FLAGS "-Wall -Wextra -Wpedantic -Werror -Wuninitialized -Wold-style-definition          \
        -Wsign-conversion -Wcast-align -Wcast-qual -Wstrict-aliasing=2 -Wpointer-arith -Warray-bounds      \
        -Wnull-dereference -Wmissing-prototypes -Wstrict-prototypes -Wconversion -Wredundant-decls -Wvla   \
        -Wshadow -Wundef -Wformat=2 -Wformat-security -Wwrite-strings -Wdouble-promotion -Wfloat-equal     \
        -Wswitch-enum -Wswitch-default -Wunused -Wunused-function -Wunused-variable -Wunused-parameter     \
        -Wno-padded -Wno-declaration-after-statement -Weverything -Wno-jump-misses-init -Wno-unsafe-buffer-usage \
        -Wno-disabled-macro-expansion -Wno-unknown-warning-option"

    #define STRICT_FLAGS_CPP "-Wall -Wextra -Wpedantic -Werror -Wuninitialized -Wsign-conversion -Wcast-align \
        -Wcast-qual -Wstrict-aliasing=2 -Wpointer-arith -Warray-bounds -Wnull-dereference -Wdouble-promotion  \
        -Wredundant-decls -Wvla -Wshadow -Wundef -Wformat=2 -Wformat-security -Wwrite-strings -Wconversion    \
        -Wfloat-equal -Wswitch-enum -Wswitch-default -Wunused -Wunused-function -Wunused-variable -Wunused-parameter \
        -Wno-padded -Weverything -Wno-jump-misses-init -Wno-unsafe-buffer-usage -Wno-disabled-macro-expansion        \
        -Wno-unknown-warning-option"
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
#endif

#if defined(_MSC_VER) && !defined(__clang__)
    #define CFLAGS_BASE "/nologo /LTCG"

    #define OPTIMIZATION_FLAGS "/O2 /Oi /Ot /GL /Gy /fp:fast"
    #define LINKER_FLAGS "/LTCG /OPT:REF /OPT:ICF"

    #define DEBUG_FLAGS_CC "/Od /Zi /RTC1 /fsanitize:address"
    #define DEBUG_FLAGS_LNK "/DEBUG /fsanitize:address"

#elif defined(_MSC_VER) && defined(__clang__)
    #define LINT_FLAGS "/clang:-Wall"
    #define MODERATE_FLAGS "/clang:-Wall /clang:-Wextra /clang:-Wpedantic /clang:-Werror /clang:-Wuninitialized /clang:-Wconversion \
        /clang:-Wsign-conversion /clang:-Wcast-align /clang:-Wstrict-aliasing=2 /clang:-Wmissing-prototypes /clang:-Wswitch-enum \
        /clang:-Wredundant-decls /clang:-Wshadow /clang:-Wundef /clang:-Wformat=2 /clang:-Wwrite-strings /clang:-Wstrict-prototypes"

    #define MODERATE_FLAGS_CPP "/clang:-Wall /clang:-Wextra /clang:-Wpedantic /clang:-Werror /clang:-Wuninitialized /clang:-Wconversion \
        /clang:-Wsign-conversion /clang:-Wcast-align /clang:-Wstrict-aliasing=2 /clang:-Wswitch-enum /clang:-Wredundant-decls \
        /clang:-Wshadow /clang:-Wundef /clang:-Wformat=2 /clang:-Wwrite-strings"

    #define CFLAGS_BASE "/nologo /LTCG"

    #define OPTIMIZATION_FLAGS "/Gy /fp:fast /clang:-march=native /clang:-ffast-math /clang:-O3"
    #define LINKER_FLAGS "/OPT:REF /OPT:ICF"

    #define DEBUG_FLAGS_CC "/Od /Zi /clang:-fno-omit-frame-pointer /clang:-fsanitize=address /clang:-fsanitize=undefined"
    #define DEBUG_FLAGS_LNK "/DEBUG /clang:-fsanitize=address /clang:-fsanitize=undefined"

#elif (defined(__clang__) || defined(__GNUC__)) && !defined(_MSC_VER)
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
    
#endif

Strictness validate_strictness(const char *level_str);

const char* delegate_strictness_flags(const Strictness level);
char* join_cflags(const CompilerOptions opts, const CompilerConfig cfg);
char* construct_ldflags(const CompilerOptions *opts, const CompilerConfig cfg);

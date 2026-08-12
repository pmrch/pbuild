#ifndef DEPS_H
#define DEPS_H

#include <stdbool.h>

#include "config.h"
#include "parser.h"

#ifdef _MSC_VER
char* locate_vcpkg_lib();
#endif

// On Unix-based systems tries linking -lmimalloc with compiler.
// On Windows dynamically tries detecting if there is an available mimalloc DLL/LIB
bool is_system_mimalloc_available(const CompilerOptions opts, const CompilerConfig cfg, char *restrict flag, const usize flag_size);

// Detects mimalloc based on paths
bool is_mimalloc_available(const CompilerOptions opts);

#endif

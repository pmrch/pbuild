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
bool is_mimalloc_available(const CompilerConfig cfg, const CompilerOptions *opts);

#endif

#ifndef DEPS_H
#define DEPS_H

#include <stdbool.h>
#include "config.h"

#ifdef _MSC_VER
char* locate_vcpkg_lib();
#endif

bool is_mimalloc_available(CompilerConfig *cfg, char *out);

#endif

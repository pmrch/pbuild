#include <stdio.h>

#include "flaghelpers.h"
#include "utils.h"
#include "log.h"

void write_std_buf(char *buf, const usize bufsize, const char *std) {
    #ifdef _MSC_VER
    snprintf(buf, bufsize, "/std:%s", std);

    #else
    snprintf(buf, bufsize, "-std=%s", std);

    #endif
}

void join_ldflags_path(char *restrict mimalloc_path, const usize destsize, const char *path) {
    #ifdef _MSC_VER
    snprintf(mimalloc_path, destsize, "/LIBPATH:%s mimalloc-static.lib", path);

    #else
    snprintf(mimalloc_path, destsize, "-L%s -lmimalloc", path);

    #endif
}


void strcat_with_space(char *restrict dest, usize dest_size, const char *restrict src) {
    if (dest == NULL) {
        LOG_ERROR("%s", "Destination buffer was NULL, can't concatenate, check logs above!");
        return;
    }

    if (src == NULL) {
        LOG_WARN("%s", "Not concatenating some flags, due to NULL value passed, check the logs above!");
        return;
    }

    strcat_cross(dest, dest_size, " ");
    strcat_cross(dest, dest_size, src);
}

void clang_clify_flags(char *restrict buf) {
    SplitString *ss = split(buf, ' ');
    if (ss == NULL) { return; }

    LOG_DEBUG("Split string into <%zu> elements", ss->num_split);
    free_split(ss);
}

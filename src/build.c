#include <stdio.h>

#include "build.h"
#include "path.h"
#include "log.h"
#include "utils.h"

/*#ifndef _MSC_VER
#include <pthread.h>
#include <string.h>

static i32 compile_one_unix(const char *const argv[], const char *cwd) {

}

#endif

static i32 compile_one(const char *const argv[], const char *cwd) {
    
}*/

static i32 prepare_build(char *restrict path, const usize buf_size, const char *cwd) {
    snprintf(path, buf_size, "%s", cwd);
    join_path(path, "build");

    if (!is_path_valid(path)) { 
        LOG_ERROR("Provided path <%s> was invalid, failed to prepare build directory!", path);
        return -1; 
    }

    if (!path_exists(path) && create_directory(path) != 0) {
        LOG_ERROR("%s", "Directory couldn't be created, failed to prepare build directory!");
        return -1;
    }

    return 0;
}

int compile_code(const char *base_cmd, const char *cwd, const isize num_jobs) {
    if (base_cmd == NULL || cwd == NULL) {
        LOG_ERROR("%s", "Can't compile code, no cwd or base command was provided!");
        return -1;
    }

    char path_buf[PATH_MAX] = { 0 };
    i32 prepared_dest = prepare_build(path_buf, sizeof(path_buf), cwd);
    if (prepared_dest != 0) { return prepared_dest; }

    (void)num_jobs;

    return 0;
}

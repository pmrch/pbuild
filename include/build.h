#ifndef BUILD_H
#define BUILD_H

// General shared imports
#include "utils.h"
#include <stdatomic.h>

// Platform-specific stuff
#ifndef _MSC_VER
#include <pthread.h>

typedef pthread_mutex_t mutex;
typedef pthread_t       thread;

#else
typedef CRITICAL_SECTION mutex;
typedef HANDLE           thread;

#endif

// Convenient definitions
typedef atomic_uint_fast16_t atomic_u16;

typedef struct {
    mutex *lock;
    char  *final_cmd;
    char  *stdout_;
    char  *stderr_;
    u16    thread_id;
} CompileArgs;

typedef struct {
    mutex     *lock;
    thread    *pool;
    atomic_u16 running;
} ThreadPool;

typedef struct {
    char **stdouts;
    char **stderrs;
} OutputBuffers;

// clang-format off
ThreadPool *create_thread_pool(usize poolsize);
// clang-format on

char *finish_compiler_command(const char *cmd, const char *src, const char *dest);
int   compile_code(const char *base_cmd, const char *cwd, const usize num_jobs);

void *compile_one(void *const args);
void  free_thread_pool(ThreadPool *tpool);
void  fill_compile_args(CompileArgs *args, mutex *mut, u16 thread_id);

#endif

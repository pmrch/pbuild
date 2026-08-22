#include <stdio.h>
#include <stddef.h>

#include "build.h"
#include "utils.h"
#include "path.h"
#include "log.h"

#ifndef _MSC_VER
#include <sys/wait.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <spawn.h>

extern char **environ;

static void *compile_one_unix(void *const args) {
    if (args == NULL) {
        LOG_ERROR("%s", "Cannot compile file, no args were provided");
        return NULL;
    }

    CompileArgs const *arguments = (CompileArgs const *)args;
    SplitString *argv = split(arguments->final_cmd, (i32)' ');
    if (argv == NULL) { 
        perror("Failed to split argv!");
        return NULL;
    }

    // Create a pipe
    //(pipefd[0] = read end, pipefd[1] = write end)
    int out_pipe[2]; // For stdout
    int err_pipe[2]; // For stderr

    if (pipe(out_pipe) < 0 || pipe(err_pipe) < 0) {
        perror("pipe failed");
        return NULL;
    }

    // Set up spawn file actions
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    // Route child's stdout (1) to out_pipe's write-end
    posix_spawn_file_actions_adddup2(&actions, out_pipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, out_pipe[1]);
    posix_spawn_file_actions_addclose(&actions, out_pipe[0]); // Child doesn't need read-end

    // Route child's stderr (2) to err_pipe's write-end
    posix_spawn_file_actions_adddup2(&actions, err_pipe[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, err_pipe[1]);
    posix_spawn_file_actions_addclose(&actions, err_pipe[0]); // Child doesn't need read-end

    pid_t pid;
    i32 status = posix_spawnp(&pid, argv->strings[0], &actions, NULL, argv->strings, environ);
    
    if (status == 0) {
        

        waitpid(pid, &status, 0);
    } else { LOG_ERROR("Failed to spawn thread <%u>", arguments->thread_id); }

    free_split(argv);
    close(out_pipe[0]);
    close(err_pipe[0]);
    return NULL;
}

static i32 compile_code_unix(DynStrArr *src_files, const char *cmd, usize num_jobs, const char *cwd) {
    if (src_files == NULL || cmd == NULL || cwd == NULL) { return -1; }
    mutex mut = PTHREAD_MUTEX_INITIALIZER;

    char destdir_stack[PATH_MAX / 2] = { 0 };
    char tmp_buf[PATH_MAX / 2] = { 0 };
    char final_buf[PATH_MAX] = { 0 };

    // Leftover slash for preventing the use of another strcat
    snprintf(destdir_stack, sizeof(destdir_stack), "%s/%s", cwd, "build"); 
    usize len = (src_files->len < num_jobs) ? src_files->len : num_jobs;

    ThreadPool *tpool = create_thread_pool(len);
    if (tpool == NULL) { return -1; }

    char **files_ptr = src_files->data;
    usize thread_id = 0;

    if (src_files->len < num_jobs) {
        for (usize i = 0; i < src_files->len; i++) {
            CompileArgs *args = (CompileArgs *)malloc(sizeof(CompileArgs));
            if (args == NULL) { free_thread_pool(tpool); return -1; }

            // Points to the start of the filename
            const char *srcfile = strrchr(*files_ptr, (i32)'/');
            if (srcfile == NULL) { free_thread_pool(tpool); return -1; }
            const char *base_start = srcfile + 1;

            const char *ext = strchr(srcfile, '.');
            if (ext != NULL) {
                usize base_len = (usize)(ext - base_start);
                mempcpy(tmp_buf, base_start, base_len);

                tmp_buf[base_len] = '\0';
                strcat_cross(tmp_buf, sizeof(tmp_buf), ".o");
            } else {
                memcpy(tmp_buf, base_start, strlen(base_start));
                tmp_buf[strlen(base_start)] = '\0';
                strcat_cross(tmp_buf, sizeof(tmp_buf), ".o");
            }

            snprintf(final_buf, sizeof(final_buf), "%s/%s", destdir_stack, tmp_buf);
            char *final_cmd = finish_compiler_command(cmd, *files_ptr++, final_buf);
            if (final_cmd == NULL) { return -1; }

            args->final_cmd = final_cmd;
            fill_compile_args(args, &mut, (u16)thread_id++);
            pthread_create(&tpool->pool[i], NULL, compile_one, (void*)args);
        }
    }


    return 0;
}

// Caller must free the returned heap-allocated buffer
static char* finish_compiler_command_unix(const char *cmd, const char *src, const char *dest) {
    if (cmd == NULL || src == NULL || dest == NULL) {
        LOG_ERROR("%s", "Couldn't create final compiler string, invalid values provided!");
        return NULL;
    }

    usize final_size = (strlen(cmd) + strlen(src) + strlen(dest)) * 2;
    char *final_command = (char *)malloc(final_size);

    if (final_command == NULL) {
        LOG_ERROR("%s", "Couldn't allocate memory for final compiler string");
        return NULL;
    }

    snprintf(final_command, final_size, "%s -c %s -o %s", cmd, src, dest);
    return final_command;
}

#else
static void *compile_one_win(void *const args)  {
    return NULL;
}

#endif

void *compile_one(void *const args) {
    #ifndef _MSC_VER
    return compile_one_unix(args);

    #else
    return compile_one_win(args);

    #endif
}

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

i32 compile_code(const char *base_cmd, const char *cwd, const usize num_jobs) {
    if (base_cmd == NULL || cwd == NULL) {
        LOG_ERROR("%s", "Can't compile code, no cwd or base command was provided!");
        return -1;
    }

    char path_buf[PATH_MAX] = { 0 };
    DynStrArr *target = new_str_vec();
    if (target == NULL) { return -1; }

    i32 prepared_dest = prepare_build(path_buf, sizeof(path_buf), cwd);
    snprintf(path_buf, sizeof(path_buf), "%s", cwd);
    join_path(path_buf, "src");

    i32 files_gathered = gather_source_files(target, path_buf);
    if (files_gathered != 0 || prepared_dest != 0) {
        free_str_vec(target);
        return -1;
    }

    // Platform specific section start
    #ifndef _MSC_VER
    compile_code_unix(target, base_cmd, num_jobs, cwd);

    #else
    compile_code_win();

    #endif

    free_str_vec(target);
    (void)num_jobs;

    return 0;
}

ThreadPool *create_thread_pool(usize poolsize) {
    ThreadPool *tpool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (tpool == NULL) {
        LOG_ERROR("%s", "Couldn't allocate memory for thread pool!");
        return  NULL;
    }

    tpool->pool = (thread*)malloc(sizeof(thread) * poolsize);
    if (tpool->pool == NULL) { 
        LOG_ERROR("%s", "Failed to initialize thread pool!");
        return NULL; 
    }

    tpool->running = 0;
    return tpool;
}

void free_thread_pool(ThreadPool *tpool) {
    if (tpool == NULL) { return; }
    if (tpool->pool != NULL) { free(tpool->pool); }
    if (tpool->lock != NULL) { free(tpool->lock); }

    free(tpool);
}


char* finish_compiler_command(const char *cmd, const char *src, const char *dest) {
    #ifndef _MSC_VER
    return finish_compiler_command_unix(cmd, src, dest);

    #else 

    #endif
}

void fill_compile_args(CompileArgs *args, mutex *mut, u16 thread_id) {
    args->lock = mut;
    args->thread_id = thread_id;
}

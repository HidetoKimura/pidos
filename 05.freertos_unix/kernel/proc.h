#pragma once
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

#define NPROC        8
#define NFD          8
#define STACK_SIZE   1024

typedef enum {
    P_EMPTY = 0,
    P_READY,
    P_RUNNING,
    P_ZOMBIE,
} proc_state_t;

typedef struct file file_t;

typedef struct {
    int      used;
    file_t*  file;
} fd_entry_t;

typedef struct {
    int           used;
    int           pid;
    int           ppid;
    proc_state_t  state;
    int           exit_code;
    TaskHandle_t  task;
    fd_entry_t    fds[NFD];
} proc_t;

typedef int (*app_entry_t)(int argc, char** argv);

void     proc_init(void);
int      proc_spawn_builtin(const char* name, int argc, char** argv, int parent_pid);
int      proc_wait_any(int* out_pid, int* out_status, TickType_t to);
void     proc_mark_exit(int pid, int status);
int      proc_kill(int pid, int status);
proc_t*  proc_get(int pid);
int      proc_get_current_pid(void);
void     proc_list(void);

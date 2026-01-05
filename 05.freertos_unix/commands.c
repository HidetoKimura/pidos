#include "commands.h"
#include <string.h>
#include <stdlib.h>

#include "basic.h"
#include "rdisk.h"
#include "exec.h"
#include "kernel/syscall.h"
#include "kernel/proc.h"
#include "kernel/vfs.h"

static int cmd_ps(int argc, char** argv) { (void)argc; (void)argv; proc_list(); return 0; }
static int cmd_runu(int argc, char** argv) {
    if (argc < 2) return -1;
    int pid = sys_spawn(argv[1], argc-1, &argv[1]);
    if (pid < 0) { console_printf("spawn failed\n"); return 0; }
    console_printf("[spawned] pid=%d\n", pid);
//    int out_pid=0, status=0; sys_wait(&out_pid, &status);
//    console_printf("[wait] pid=%d status=%d\n", out_pid, status);
    return 0;
}
static int cmd_wait(int argc, char** argv) {
    int pid=0, status=0;
    if (argc == 1) {
        sys_wait(&pid, &status);
        console_printf("[wait] pid=%d status=%d\n", pid, status);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "-n") == 0) {
        int r = sys_wait_nowait(&pid, &status);
        if (r == 0) {
            console_printf("[wait] no exited process\n");
        } else {
            console_printf("[wait] pid=%d status=%d\n", pid, status);
        }
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "-t") == 0) {
        int ms = atoi(argv[2]);
        if (ms < 0) ms = 0;
        int r = sys_wait_timeout(&pid, &status, (uint32_t)ms);
        if (r == 0) {
            console_printf("[wait] timeout\n");
        } else {
            console_printf("[wait] pid=%d status=%d\n", pid, status);
        }
        return 0;
    }
    return -1;
}
static int cmd_cat(int argc, char** argv) {
    if (argc != 2) return -1;
    int fd = sys_open(argv[1]);
    if (fd < 0) { console_printf("open failed\n"); return 0; }
    char buf[128];
    int n = sys_read(fd, buf, sizeof(buf));
    if (n > 0) sys_write(1, buf, n);
    sys_close(fd);
    return 0;
}

static int cmd_kill(int argc, char** argv) {
    if (argc != 2 && argc != 3) return -1;
    int status = -9;
    int pid_idx = 1;
    if (argc == 3) {
        status = atoi(argv[1]);
        pid_idx = 2;
    }
    int pid = atoi(argv[pid_idx]);
    int r = sys_kill(pid, status);
    if (r < 0) console_printf("kill failed pid=%d\n", pid);
    return 0;
}

CommandList commands[] = {
    {basic_cmd_help,    "help",             "help"                                             },    
    {basic_cmd_led,     "led",              "led <on|off>"                                     }, 
    {rdisk_cmd_format,  "format",           "format"                                           },    
    {rdisk_cmd_dir,     "dir",              "dir"                                              },    
    {rdisk_cmd_save,    "save",             "save <filename> <data>"                           },    
    {rdisk_cmd_delete,  "delete",           "delete <filename>"                                },    
    {rdisk_cmd_type,    "type",             "type <filename>"                                  },    
    {rdisk_cmd_loadhex, "loadhex",          "loadhex <name> <caphex>"                          },    
    {exec_cmd_run,      "run",              "run <name> [addrhex]"                             },    
    /* Unix-like minimal commands */
    {basic_cmd_help,    "help",             "help"                                             },
    {cmd_ps,            "ps",               "ps"                                               },
    {cmd_runu,          "runu",             "runu <app> [args...]"                             },
    {cmd_wait,          "wait",             "wait [-n | -t <ms>]"                              },
    {cmd_cat,           "cat",              "cat <path>"                                       },
    {cmd_kill,          "kill",             "kill [status] <pid>"                              },
    {NULL,              "",                 ""                                                 }  // End marker                                            
};

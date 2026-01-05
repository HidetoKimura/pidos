#pragma once
#include <stdint.h>
#include "proc.h"
#include "kernel/vfs.h"

int sys_write(int fd, const void* buf, int len);
int sys_read(int fd, void* buf, int len);
int sys_open(const char* path);
int sys_close(int fd);
int sys_exit(int code);
int sys_getpid(void);
int sys_spawn(const char* name, int argc, char** argv);
int sys_wait(int* out_pid, int* out_status);
int sys_wait_timeout(int* out_pid, int* out_status, uint32_t timeout_ms);
int sys_wait_nowait(int* out_pid, int* out_status);
int sys_kill(int pid, int status);
void sys_init(void);

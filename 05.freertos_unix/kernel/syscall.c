#include <string.h>
#include "syscall.h"
#include "kernel/vfs.h"
#include "console.h"
#include "FreeRTOS.h"

/* Per-process fd table lives in proc_t */

static file_t* g_global_fd[NFD]; /* simple shared fd table for minimal demo */

void sys_init(void)
{
    vfs_init();
    /* stdin/out/err */
    g_global_fd[0] = vfs_open("/dev/tty");
    g_global_fd[1] = vfs_open("/dev/tty");
    g_global_fd[2] = vfs_open("/dev/tty");
}

int sys_open(const char* path)
{
    for (int i = 3; i < NFD; i++) {
        if (!g_global_fd[i]) {
            file_t* f = vfs_open(path);
            if (!f) return -1;
            g_global_fd[i] = f;
            return i;
        }
    }
    return -2;
}

int sys_close(int fd)
{
    if (fd < 0 || fd >= NFD || !g_global_fd[fd]) return -1;
    vfs_close(g_global_fd[fd]);
    g_global_fd[fd] = NULL;
    return 0;
}

int sys_write(int fd, const void* buf, int len)
{
    if (fd < 0 || fd >= NFD || !g_global_fd[fd]) return -1;
    return vfs_write(g_global_fd[fd], buf, len);
}

int sys_read(int fd, void* buf, int len)
{
    if (fd < 0 || fd >= NFD || !g_global_fd[fd]) return -1;
    return vfs_read(g_global_fd[fd], buf, len);
}

int sys_getpid(void)
{
    return proc_get_current_pid();
}

int sys_exit(int code)
{
    int pid = proc_get_current_pid();
    proc_mark_exit(pid, code);
    vTaskDelete(NULL);
    return 0;
}

int sys_spawn(const char* name, int argc, char** argv)
{
    int ppid = proc_get_current_pid();
    return proc_spawn_builtin(name, argc, argv, ppid);
}

int sys_wait(int* out_pid, int* out_status)
{
    return proc_wait_any(out_pid, out_status, portMAX_DELAY);
}

int sys_wait_timeout(int* out_pid, int* out_status, uint32_t timeout_ms)
{
    TickType_t to = (timeout_ms == (uint32_t)0xFFFFFFFFu)
                    ? portMAX_DELAY
                    : pdMS_TO_TICKS(timeout_ms);
    return proc_wait_any(out_pid, out_status, to);
}

int sys_wait_nowait(int* out_pid, int* out_status)
{
    return proc_wait_any(out_pid, out_status, 0);
}

int sys_kill(int pid, int status)
{
    return proc_kill(pid, status);
}

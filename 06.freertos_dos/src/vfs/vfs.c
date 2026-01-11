// vfs.c
#include "vfs.h"
#include "fs/ramfs.h"
#include "util/strutil.h"
#include "pico/stdio.h"
#include <string.h>

typedef enum { FD_FREE=0, FD_CON, FD_NUL, FD_RAMFILE } fd_kind_t;

typedef struct {
    fd_kind_t kind;
    int mode;
    int handle; // ramfs handle
} fd_ent_t;

#define VFS_MAX_FD 8
static fd_ent_t g_fd[VFS_MAX_FD];

static int alloc_fd(void) {
    for (int i = 0; i < VFS_MAX_FD; i++) {
        if (g_fd[i].kind == FD_FREE) return i;
    }
    return -1;
}

bool vfs_is_device_path(const char* path) {
    // "CON:" or "NUL:"
    if (!path) return false;
    return (path[0] && path[1] && path[2] && path[3] == ':');
}

void vfs_init(void) {
    for (int i = 0; i < VFS_MAX_FD; i++) {
        g_fd[i].kind = FD_FREE;
        g_fd[i].mode = 0;
        g_fd[i].handle = -1;
    }
    // fd 0/1/2 = CON
    g_fd[0].kind = FD_CON; g_fd[0].mode = VFS_O_RDONLY;
    g_fd[1].kind = FD_CON; g_fd[1].mode = VFS_O_WRONLY;
    g_fd[2].kind = FD_CON; g_fd[2].mode = VFS_O_WRONLY;
}

static bool is_con(const char* p){ return str_eq_nocase(p, "CON:"); }
static bool is_nul(const char* p){ return str_eq_nocase(p, "NUL:"); }

int vfs_open(const char* path, int mode, vfs_err_t* err) {
    if (err) *err = VFS_OK;
    if (!path) { if (err) *err = VFS_E_INVAL; return -1; }

    int fd = alloc_fd();
    if (fd < 0) { if (err) *err = VFS_E_BUSY; return -1; }

    if (is_con(path)) {
        g_fd[fd] = (fd_ent_t){ .kind=FD_CON, .mode=mode, .handle=-1 };
        return fd;
    }
    if (is_nul(path)) {
        g_fd[fd] = (fd_ent_t){ .kind=FD_NUL, .mode=mode, .handle=-1 };
        return fd;
    }

    // A:\... only
    int h = ramfs_open(path, mode, err);
    if (h < 0) return -1;
    g_fd[fd] = (fd_ent_t){ .kind=FD_RAMFILE, .mode=mode, .handle=h };
    return fd;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= VFS_MAX_FD) return -1;
    if (g_fd[fd].kind == FD_RAMFILE) ramfs_close(g_fd[fd].handle);
    if (fd >= 3) g_fd[fd].kind = FD_FREE;
    return 0;
}

int vfs_read(int fd, void* buf, size_t len, vfs_err_t* err) {
    if (err) *err = VFS_OK;
    if (fd < 0 || fd >= VFS_MAX_FD) { if (err) *err = VFS_E_INVAL; return -1; }

    switch (g_fd[fd].kind) {
    case FD_CON: return 0; // CONのreadはshell側で扱う（ここは未使用）
    case FD_NUL: return 0;
    case FD_RAMFILE: return ramfs_read(g_fd[fd].handle, buf, len, err);
    default: if (err) *err = VFS_E_INVAL; return -1;
    }
}

int vfs_write(int fd, const void* buf, size_t len, vfs_err_t* err) {
    if (err) *err = VFS_OK;
    if (fd < 0 || fd >= VFS_MAX_FD) { if (err) *err = VFS_E_INVAL; return -1; }

    switch (g_fd[fd].kind) {
    case FD_CON:
        // CRLF寄せ（DOSっぽく）
        for (size_t i=0;i<len;i++){
            char c = ((const char*)buf)[i];
            if (c == '\n') { putchar_raw('\r'); putchar_raw('\n'); }
            else putchar_raw(c);
        }
        return (int)len;
    case FD_NUL:
        return (int)len;
    case FD_RAMFILE:
        return ramfs_write(g_fd[fd].handle, buf, len, err);
    default:
        if (err) *err = VFS_E_INVAL;
        return -1;
    }
}

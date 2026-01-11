// vfs.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    VFS_OK = 0,
    VFS_E_NOENT,
    VFS_E_INVAL,
    VFS_E_NOSPC,
    VFS_E_IO,
    VFS_E_BUSY,
} vfs_err_t;

typedef enum {
    VFS_O_RDONLY = 1,
    VFS_O_WRONLY = 2,
    VFS_O_RDWR   = 3,
    VFS_O_CREAT  = 1 << 8,
    VFS_O_TRUNC  = 1 << 9,
} vfs_open_mode_t;

void vfs_init(void);

int vfs_open(const char* path, int mode, vfs_err_t* err);
int vfs_close(int fd);

int vfs_read(int fd, void* buf, size_t len, vfs_err_t* err);
int vfs_write(int fd, const void* buf, size_t len, vfs_err_t* err);

bool vfs_is_device_path(const char* path);

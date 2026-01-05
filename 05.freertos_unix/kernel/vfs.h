#pragma once
#include <stdint.h>

typedef struct file file_t;

typedef struct {
    int  (*read)(file_t* f, void* buf, int len);
    int  (*write)(file_t* f, const void* buf, int len);
    int  (*close)(file_t* f);
} file_ops_t;

struct file {
    const char*   path;
    file_ops_t*   ops;
    void*         priv;
};

int      vfs_init(void);
file_t*  vfs_open(const char* path);
int      vfs_close(file_t* f);
int      vfs_read(file_t* f, void* buf, int len);
int      vfs_write(file_t* f, const void* buf, int len);

// ramfs.h
#pragma once
#include "vfs/vfs.h"
#include <stddef.h>
#include <stdbool.h>

void ramfs_init(void);

// vfsから呼ばれる
int  ramfs_open(const char* path, int mode, vfs_err_t* err);
int  ramfs_close(int handle);
int  ramfs_read(int handle, void* buf, size_t len, vfs_err_t* err);
int  ramfs_write(int handle, const void* buf, size_t len, vfs_err_t* err);

// シェル用
bool ramfs_list(int idx, char* name_out, size_t name_cap, size_t* size_out, bool* used_out);
bool ramfs_delete(const char* path, vfs_err_t* err);

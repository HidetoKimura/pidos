// ramfs.h（差し替え/追記）
#pragma once
#include "vfs/vfs.h"
#include <stddef.h>
#include <stdbool.h>

void ramfs_init(void);

// cwd 操作
int  ramfs_get_cwd_node(void);
bool ramfs_cd(const char* path, vfs_err_t* err);
bool ramfs_pwd(char* out, size_t cap);

// ディレクトリ操作
bool ramfs_mkdir(const char* path, vfs_err_t* err);
bool ramfs_rmdir(const char* path, vfs_err_t* err);

// ファイル操作（VFSから）
int  ramfs_open(const char* path, int mode, vfs_err_t* err);
int  ramfs_close(int handle);
int  ramfs_read(int handle, void* buf, size_t len, vfs_err_t* err);
int  ramfs_write(int handle, const void* buf, size_t len, vfs_err_t* err);
bool ramfs_delete(const char* path, vfs_err_t* err);

// DIR表示用：指定ディレクトリ（NULLならcwd）を列挙
typedef struct {
    bool used;
    bool is_dir;
    char name[16];
    size_t size;
} ramfs_dirent_t;

bool ramfs_list_dir(const char* path_or_null, int idx, ramfs_dirent_t* out, vfs_err_t* err);

size_t ramfs_serialize(uint8_t *out, size_t cap);
bool   ramfs_deserialize(const uint8_t *in, size_t len);

// dirtyフラグ（保存タイミング制御用）
bool ramfs_is_dirty(void);
void ramfs_set_dirty(void);
void ramfs_clear_dirty(void);


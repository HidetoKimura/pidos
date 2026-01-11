// ramfs.c
#include "ramfs.h"
#include "util/strutil.h"
#include <string.h>

#define RAMFS_MAX_FILES 16
#define RAMFS_NAME_CAP  16      // "FOO.TXT" など
#define RAMFS_FILE_CAP  1024    // 1ファイル最大1KB（教育用）

typedef struct {
    bool used;
    char name[RAMFS_NAME_CAP]; // uppercase
    size_t size;
    uint8_t data[RAMFS_FILE_CAP];
} ramfile_t;

typedef struct {
    bool used;
    int file_idx;
    size_t pos;
    int mode;
} ramfh_t;

static ramfile_t g_files[RAMFS_MAX_FILES];
static ramfh_t   g_fh[8];

static bool normalize_path_to_name(const char* path, char* out, size_t cap) {
    // Accept: "A:\\FOO.TXT" or "A:FOO.TXT" or "FOO.TXT"
    if (!path || !out || cap < 2) return false;

    const char* p = path;

    // skip drive
    if ((p[0] && p[1] == ':')) {
        p += 2;
        if (*p == '\\' || *p == '/') p++;
    }
    // now p points name
    if (!*p) return false;

    // copy until end or path separator (no subdirs in this minimal ramfs)
    size_t n = 0;
    while (p[n] && p[n] != '\\' && p[n] != '/') n++;
    if (n == 0 || n >= cap) return false;

    memcpy(out, p, n);
    out[n] = '\0';
    str_to_upper(out);
    return true;
}

static int find_file_by_name(const char* name) {
    for (int i=0;i<RAMFS_MAX_FILES;i++){
        if (g_files[i].used && str_eq_nocase(g_files[i].name, name)) return i;
    }
    return -1;
}

static int alloc_file_slot(void) {
    for (int i=0;i<RAMFS_MAX_FILES;i++) if (!g_files[i].used) return i;
    return -1;
}

static int alloc_fh(void) {
    for (int i=0;i<(int)(sizeof(g_fh)/sizeof(g_fh[0]));i++) if (!g_fh[i].used) return i;
    return -1;
}

void ramfs_init(void) {
    memset(g_files, 0, sizeof(g_files));
    memset(g_fh, 0, sizeof(g_fh));

    // 初期ファイル（例）
    g_files[0].used = true;
    strncpy(g_files[0].name, "README.TXT", RAMFS_NAME_CAP-1);
    const char* msg = "Welcome to PicoDOS.\r\nTry: DIR, TYPE README.TXT, RUN HELLO\r\n";
    g_files[0].size = strlen(msg);
    memcpy(g_files[0].data, msg, g_files[0].size);
}

int ramfs_open(const char* path, int mode, vfs_err_t* err) {
    if (err) *err = VFS_OK;

    char name[RAMFS_NAME_CAP];
    if (!normalize_path_to_name(path, name, sizeof(name))) {
        if (err) *err = VFS_E_INVAL;
        return -1;
    }

    int idx = find_file_by_name(name);

    bool want_creat = (mode & VFS_O_CREAT) != 0;
    bool want_trunc = (mode & VFS_O_TRUNC) != 0;

    if (idx < 0) {
        if (!want_creat) { if (err) *err = VFS_E_NOENT; return -1; }
        idx = alloc_file_slot();
        if (idx < 0) { if (err) *err = VFS_E_NOSPC; return -1; }
        g_files[idx].used = true;
        strncpy(g_files[idx].name, name, RAMFS_NAME_CAP-1);
        g_files[idx].size = 0;
    }

    if (want_trunc) g_files[idx].size = 0;

    int fh = alloc_fh();
    if (fh < 0) { if (err) *err = VFS_E_BUSY; return -1; }
    g_fh[fh].used = true;
    g_fh[fh].file_idx = idx;
    g_fh[fh].pos = 0;
    g_fh[fh].mode = mode;
    return fh;
}

int ramfs_close(int handle) {
    if (handle < 0 || handle >= (int)(sizeof(g_fh)/sizeof(g_fh[0]))) return -1;
    g_fh[handle].used = false;
    return 0;
}

int ramfs_read(int handle, void* buf, size_t len, vfs_err_t* err) {
    if (err) *err = VFS_OK;
    if (handle < 0 || handle >= (int)(sizeof(g_fh)/sizeof(g_fh[0])) || !g_fh[handle].used) {
        if (err) *err = VFS_E_INVAL; return -1;
    }
    ramfh_t* h = &g_fh[handle];
    ramfile_t* f = &g_files[h->file_idx];

    size_t remain = (h->pos < f->size) ? (f->size - h->pos) : 0;
    if (remain == 0) return 0;

    if (len > remain) len = remain;
    memcpy(buf, f->data + h->pos, len);
    h->pos += len;
    return (int)len;
}

int ramfs_write(int handle, const void* buf, size_t len, vfs_err_t* err) {
    if (err) *err = VFS_OK;
    if (handle < 0 || handle >= (int)(sizeof(g_fh)/sizeof(g_fh[0])) || !g_fh[handle].used) {
        if (err) *err = VFS_E_INVAL; return -1;
    }
    ramfh_t* h = &g_fh[handle];
    ramfile_t* f = &g_files[h->file_idx];

    if (h->pos + len > RAMFS_FILE_CAP) {
        if (err) *err = VFS_E_NOSPC;
        // 書ける分だけでもOKにしたいならここで切る
        return -1;
    }

    memcpy(f->data + h->pos, buf, len);
    h->pos += len;
    if (h->pos > f->size) f->size = h->pos;
    return (int)len;
}

bool ramfs_list(int idx, char* name_out, size_t name_cap, size_t* size_out, bool* used_out) {
    if (idx < 0 || idx >= RAMFS_MAX_FILES) return false;
    if (used_out) *used_out = g_files[idx].used;
    if (!g_files[idx].used) return true;

    if (name_out && name_cap) {
        strncpy(name_out, g_files[idx].name, name_cap - 1);
        name_out[name_cap - 1] = '\0';
    }
    if (size_out) *size_out = g_files[idx].size;
    return true;
}

bool ramfs_delete(const char* path, vfs_err_t* err) {
    if (err) *err = VFS_OK;
    char name[RAMFS_NAME_CAP];
    if (!normalize_path_to_name(path, name, sizeof(name))) {
        if (err) *err = VFS_E_INVAL; return false;
    }
    int idx = find_file_by_name(name);
    if (idx < 0) { if (err) *err = VFS_E_NOENT; return false; }
    g_files[idx].used = false;
    g_files[idx].size = 0;
    return true;
}

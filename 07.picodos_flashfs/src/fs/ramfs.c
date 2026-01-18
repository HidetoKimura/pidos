// ramfs.c（丸ごと差し替え推奨）
#include "ramfs.h"
#include "util/strutil.h"
#include <string.h>
#include <stdio.h>

#define RAMFS_MAX_NODES 64
#define RAMFS_NAME_CAP  16
#define RAMFS_FILE_CAP  1024
#define RAMFS_MAX_FH    8

typedef enum { N_FREE=0, N_DIR, N_FILE } ntype_t;

typedef struct {
    ntype_t type;
    bool used;
    int parent;        // -1 for root
    int first_child;   // node index or -1
    int next_sibling;  // node index or -1
    char name[RAMFS_NAME_CAP]; // uppercase
    // file data
    size_t size;
    unsigned char data[RAMFS_FILE_CAP];
} node_t;

typedef struct {
    bool used;
    int node;      // file node index
    size_t pos;
    int mode;
} fh_t;

static node_t g_nodes[RAMFS_MAX_NODES];
static fh_t   g_fh[RAMFS_MAX_FH];

static int g_root = 0;
static int g_cwd  = 0;

static bool g_dirty = false;
bool ramfs_is_dirty(void){ return g_dirty; }
void ramfs_set_dirty(void){ g_dirty = true; }
void ramfs_clear_dirty(void){ g_dirty = false; }

// mkdir/delete/write など状態を変える箇所の最後で ramfs_set_dirty(); を呼ぶ

static int alloc_node(void) {
    for (int i=0;i<RAMFS_MAX_NODES;i++){
        if (!g_nodes[i].used) return i;
    }
    return -1;
}

static int alloc_fh(void) {
    for (int i=0;i<RAMFS_MAX_FH;i++){
        if (!g_fh[i].used) return i;
    }
    return -1;
}

static void link_child(int parent, int child) {
    // prepend
    g_nodes[child].next_sibling = g_nodes[parent].first_child;
    g_nodes[parent].first_child = child;
    g_nodes[child].parent = parent;
}

static int find_child(int parent, const char* name_upper) {
    for (int c = g_nodes[parent].first_child; c != -1; c = g_nodes[c].next_sibling) {
        if (g_nodes[c].used && str_eq_nocase(g_nodes[c].name, name_upper)) return c;
    }
    return -1;
}

static bool parse_path(const char* path, const char** p_out, int* start_dir_out) {
    // Accept: "A:\foo\bar" "A:foo\bar" "\foo" "foo\bar"
    if (!path) return false;
    const char* p = path;

    // drive
    if (p[0] && p[1] == ':') {
        // only A: supported in this ramfs
        p += 2;
    }
    // absolute?
    bool abs = (*p == '\\' || *p == '/');
    if (abs) p++;

    *p_out = p;
    *start_dir_out = abs ? g_root : g_cwd;
    return true;
}

static int walk_dir(const char* path, vfs_err_t* err) {
    if (err) *err = VFS_OK;

    const char* p;
    int dir;
    if (!parse_path(path, &p, &dir)) { if (err) *err = VFS_E_INVAL; return -1; }

    if (!*p) return dir; // "A:\" or "" -> current dir

    char part[RAMFS_NAME_CAP];
    while (*p) {
        // extract part until \ or /
        size_t n=0;
        while (p[n] && p[n] != '\\' && p[n] != '/') {
            if (n+1 < sizeof(part)) part[n] = p[n];
            n++;
        }
        if (n >= sizeof(part)) { if (err) *err = VFS_E_INVAL; return -1; }
        part[n] = '\0';
        str_to_upper(part);

        // special
        if (strcmp(part, ".") == 0) {
            // noop
        } else if (strcmp(part, "..") == 0) {
            if (dir != g_root) dir = g_nodes[dir].parent;
        } else {
            int c = find_child(dir, part);
            if (c < 0 || g_nodes[c].type != N_DIR) { if (err) *err = VFS_E_NOENT; return -1; }
            dir = c;
        }

        if (!p[n]) break;
        p += n + 1;
        if (!*p) break;
    }
    return dir;
}

// path -> (parent_dir, leaf_name_upper)
static bool split_parent_leaf(const char* path, int* parent_out, char* leaf_out, size_t leaf_cap, vfs_err_t* err) {
    if (err) *err = VFS_OK;

    const char* p;
    int start;
    if (!parse_path(path, &p, &start)) { if (err) *err = VFS_E_INVAL; return false; }
    if (!*p) { if (err) *err = VFS_E_INVAL; return false; }

    // find last separator
    const char* last_sep = NULL;
    for (const char* q = p; *q; q++) {
        if (*q == '\\' || *q == '/') last_sep = q;
    }

    if (!last_sep) {
        // leaf only in start dir
        *parent_out = start;
        strncpy(leaf_out, p, leaf_cap-1);
        leaf_out[leaf_cap-1] = '\0';
        str_to_upper(leaf_out);
        return true;
    }

    // parent path = p .. last_sep-1
    char parent_path[96];
    size_t plen = (size_t)(last_sep - (p - 0));
    // careful: build a synthetic path relative to drive handling:
    // easiest: create temp by copying original then trunc.
    // We'll just copy original 'path' and cut after last_sep.
    char tmp[128];
    strncpy(tmp, path, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = '\0';

    // truncate after last separator (keep it as end)
    // find same last_sep position in tmp
    size_t pos = (size_t)(last_sep - p);
    // locate p within tmp: we don't have pointer mapping; easiest: recompute by scanning tmp for last sep from end
    int cut = -1;
    for (int i=(int)strlen(tmp)-1;i>=0;i--){
        if (tmp[i]=='\\' || tmp[i]=='/') { cut=i; break; }
    }
    if (cut < 0) { if (err) *err = VFS_E_INVAL; return false; }
    tmp[cut] = '\0';

    int parent_dir = walk_dir(tmp, err);
    if (parent_dir < 0) return false;

    const char* leaf = last_sep + 1;
    if (!*leaf) { if (err) *err = VFS_E_INVAL; return false; }

    strncpy(leaf_out, leaf, leaf_cap-1);
    leaf_out[leaf_cap-1] = '\0';
    str_to_upper(leaf_out);

    *parent_out = parent_dir;
    return true;
}

void ramfs_init(void) {
    memset(g_nodes, 0, sizeof(g_nodes));
    memset(g_fh, 0, sizeof(g_fh));

    // root node at 0
    g_root = 0;
    g_nodes[g_root].used = true;
    g_nodes[g_root].type = N_DIR;
    g_nodes[g_root].parent = -1;
    g_nodes[g_root].first_child = -1;
    g_nodes[g_root].next_sibling = -1;
    strcpy(g_nodes[g_root].name, ""); // root name empty
    g_cwd = g_root;

    // create README.TXT in root
    int f = alloc_node();
    g_nodes[f].used = true;
    g_nodes[f].type = N_FILE;
    g_nodes[f].first_child = -1;
    g_nodes[f].next_sibling = -1;
    strcpy(g_nodes[f].name, "README.TXT");
    const char* msg = "Welcome to PicoDOS.\r\nTry: DIR, MD TEST, CD TEST\r\n";
    g_nodes[f].size = strlen(msg);
    memcpy(g_nodes[f].data, msg, g_nodes[f].size);
    link_child(g_root, f);
}

int ramfs_get_cwd_node(void) { return g_cwd; }

bool ramfs_pwd(char* out, size_t cap) {
    if (!out || cap < 4) return false;

    // build reverse list
    int stack[16];
    int sp=0;
    int n = g_cwd;
    while (n != -1 && sp < 16) {
        stack[sp++] = n;
        n = g_nodes[n].parent;
    }

    // root => "A:\"
    size_t w=0;
    w += (size_t)snprintf(out+w, cap-w, "A:\\");
    // from root child to cwd
    for (int i=sp-2; i>=0; i--) {
        const char* name = g_nodes[stack[i]].name;
        if (!name[0]) continue;
        w += (size_t)snprintf(out+w, cap-w, "%s", name);
        if (i != 0) w += (size_t)snprintf(out+w, cap-w, "\\");
        if (w >= cap) break;
    }
    return true;
}

bool ramfs_cd(const char* path, vfs_err_t* err) {
    int dir = walk_dir(path, err);
    if (dir < 0) return false;
    g_cwd = dir;
    return true;
}

bool ramfs_mkdir(const char* path, vfs_err_t* err) {
    int parent;
    char leaf[RAMFS_NAME_CAP];
    if (!split_parent_leaf(path, &parent, leaf, sizeof(leaf), err)) return false;

    if (find_child(parent, leaf) >= 0) { if (err) *err = VFS_E_INVAL; return false; }

    int n = alloc_node();
    if (n < 0) { if (err) *err = VFS_E_NOSPC; return false; }

    g_nodes[n].used = true;
    g_nodes[n].type = N_DIR;
    g_nodes[n].first_child = -1;
    g_nodes[n].next_sibling = -1;
    strncpy(g_nodes[n].name, leaf, RAMFS_NAME_CAP-1);
    g_nodes[n].name[RAMFS_NAME_CAP-1] = '\0';
    link_child(parent, n);

    ramfs_set_dirty();

    return true;
}

static bool is_dir_empty(int dir) {
    return g_nodes[dir].first_child == -1;
}

bool ramfs_rmdir(const char* path, vfs_err_t* err) {
    // find node itself (must be dir)
    int parent;
    char leaf[RAMFS_NAME_CAP];
    if (!split_parent_leaf(path, &parent, leaf, sizeof(leaf), err)) return false;
    int d = find_child(parent, leaf);
    if (d < 0 || g_nodes[d].type != N_DIR) { if (err) *err = VFS_E_NOENT; return false; }
    if (d == g_root) { if (err) *err = VFS_E_INVAL; return false; }
    if (!is_dir_empty(d)) { if (err) *err = VFS_E_BUSY; return false; }

    // unlink from parent's child list
    int* pp = &g_nodes[parent].first_child;
    while (*pp != -1) {
        if (*pp == d) { *pp = g_nodes[d].next_sibling; break; }
        pp = &g_nodes[*pp].next_sibling;
    }
    g_nodes[d].used = false;

    ramfs_set_dirty();
    return true;
}

bool ramfs_delete(const char* path, vfs_err_t* err) {
    int parent;
    char leaf[RAMFS_NAME_CAP];
    if (!split_parent_leaf(path, &parent, leaf, sizeof(leaf), err)) return false;
    int f = find_child(parent, leaf);
    if (f < 0 || g_nodes[f].type != N_FILE) { if (err) *err = VFS_E_NOENT; return false; }

    // unlink
    int* pp = &g_nodes[parent].first_child;
    while (*pp != -1) {
        if (*pp == f) { *pp = g_nodes[f].next_sibling; break; }
        pp = &g_nodes[*pp].next_sibling;
    }
    g_nodes[f].used = false;
    g_nodes[f].size = 0;

    ramfs_set_dirty();

    return true;
}

// ---- file handles ----

int ramfs_open(const char* path, int mode, vfs_err_t* err) {
    if (err) *err = VFS_OK;

    int parent;
    char leaf[RAMFS_NAME_CAP];
    if (!split_parent_leaf(path, &parent, leaf, sizeof(leaf), err)) return -1;

    int n = find_child(parent, leaf);
    bool want_creat = (mode & VFS_O_CREAT) != 0;
    bool want_trunc = (mode & VFS_O_TRUNC) != 0;

    if (n < 0) {
        if (!want_creat) { if (err) *err = VFS_E_NOENT; return -1; }
        n = alloc_node();
        if (n < 0) { if (err) *err = VFS_E_NOSPC; return -1; }
        g_nodes[n].used = true;
        g_nodes[n].type = N_FILE;
        g_nodes[n].first_child = -1;
        g_nodes[n].next_sibling = -1;
        strncpy(g_nodes[n].name, leaf, RAMFS_NAME_CAP-1);
        g_nodes[n].name[RAMFS_NAME_CAP-1] = '\0';
        g_nodes[n].size = 0;
        link_child(parent, n);
    } else {
        if (g_nodes[n].type != N_FILE) { if (err) *err = VFS_E_INVAL; return -1; }
        if (want_trunc) g_nodes[n].size = 0;
    }

    int fh = alloc_fh();
    if (fh < 0) { if (err) *err = VFS_E_BUSY; return -1; }
    g_fh[fh].used = true;
    g_fh[fh].node = n;
    g_fh[fh].pos = (mode & VFS_O_APPEND) ? g_nodes[n].size : 0;
    g_fh[fh].mode = mode;
    return fh;
}

int ramfs_close(int handle) {
    if (handle < 0 || handle >= RAMFS_MAX_FH) return -1;
    g_fh[handle].used = false;
    return 0;
}

int ramfs_read(int handle, void* buf, size_t len, vfs_err_t* err) {
    if (err) *err = VFS_OK;
    if (handle < 0 || handle >= RAMFS_MAX_FH || !g_fh[handle].used) { if (err) *err = VFS_E_INVAL; return -1; }
    node_t* f = &g_nodes[g_fh[handle].node];
    size_t pos = g_fh[handle].pos;
    if (pos >= f->size) return 0;
    size_t remain = f->size - pos;
    if (len > remain) len = remain;
    memcpy(buf, f->data + pos, len);
    g_fh[handle].pos += len;
    return (int)len;
}

int ramfs_write(int handle, const void* buf, size_t len, vfs_err_t* err) {
    if (err) *err = VFS_OK;
    if (handle < 0 || handle >= RAMFS_MAX_FH || !g_fh[handle].used) { if (err) *err = VFS_E_INVAL; return -1; }
    node_t* f = &g_nodes[g_fh[handle].node];
    size_t pos = g_fh[handle].pos;
    if (pos + len > RAMFS_FILE_CAP) { if (err) *err = VFS_E_NOSPC; return -1; }
    memcpy(f->data + pos, buf, len);
    g_fh[handle].pos += len;
    if (g_fh[handle].pos > f->size) f->size = g_fh[handle].pos;

    ramfs_set_dirty();

    return (int)len;
}

// ---- directory listing ----

bool ramfs_list_dir(const char* path_or_null, int idx, ramfs_dirent_t* out, vfs_err_t* err) {
    if (err) *err = VFS_OK;
    if (!out) { if (err) *err = VFS_E_INVAL; return false; }

    int dir = g_cwd;
    if (path_or_null && path_or_null[0]) {
        dir = walk_dir(path_or_null, err);
        if (dir < 0) return false;
    }

    int c = g_nodes[dir].first_child;
    // advance idx-th used child
    int k = 0;
    while (c != -1) {
        if (g_nodes[c].used) {
            if (k == idx) {
                out->used = true;
                out->is_dir = (g_nodes[c].type == N_DIR);
                strncpy(out->name, g_nodes[c].name, sizeof(out->name)-1);
                out->name[sizeof(out->name)-1] = '\0';
                out->size = (g_nodes[c].type == N_FILE) ? g_nodes[c].size : 0;
                return true;
            }
            k++;
        }
        c = g_nodes[c].next_sibling;
    }
    out->used = false;
    return true;
}

// ---- serialization / deserialization ----
typedef struct {
    uint32_t version;
    int32_t root;
    int32_t cwd;
    node_t  nodes[RAMFS_MAX_NODES];
} ramfs_image_t;

size_t ramfs_serialize(uint8_t *out, size_t cap) {
    ramfs_image_t img;
    img.version = 1;
    img.root = g_root;
    img.cwd  = g_cwd;
    memcpy(img.nodes, g_nodes, sizeof(g_nodes));

    if (cap < sizeof(img)) return 0;
    memcpy(out, &img, sizeof(img));
    return sizeof(img);
}

bool ramfs_deserialize(const uint8_t *in, size_t len) {
    if (len != sizeof(ramfs_image_t)) return false;
    ramfs_image_t img;
    memcpy(&img, in, sizeof(img));
    if (img.version != 1) return false;

    memcpy(g_nodes, img.nodes, sizeof(g_nodes));
    g_root = img.root;
    g_cwd  = img.cwd;

    // 最小の整合性チェック
    if (g_root < 0 || g_root >= RAMFS_MAX_NODES) return false;
    if (!g_nodes[g_root].used || g_nodes[g_root].type != N_DIR) return false;
    if (g_cwd < 0 || g_cwd >= RAMFS_MAX_NODES || !g_nodes[g_cwd].used) g_cwd = g_root;

    // 復元直後はdirtyではない
    g_dirty = false;
    return true;
}

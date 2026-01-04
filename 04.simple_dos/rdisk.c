#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "rdisk.h"

#ifndef RDISK_MAX_FILES
#define RDISK_MAX_FILES  32
#endif

#ifndef RDISK_NAME_MAX
#define RDISK_NAME_MAX   16   // 15 + '\0'
#endif

typedef struct {
    char     name[RDISK_NAME_MAX]; // null-terminated
    uint32_t size;                 // bytes (actual)
    uint32_t cap;                  // bytes (allocated)
    uint32_t data_off;             // offset from disk base
    uint32_t flags;                // bit0=used
} rd_file_t;

typedef struct {
    uint32_t magic;        // "RDK1"
    uint32_t version;      // 1
    uint32_t disk_size;    // bytes
    uint32_t num_files;    // used entries
    uint32_t next_free;    // next free offset (bump)
    uint32_t reserved[3];
    rd_file_t files[RDISK_MAX_FILES];
} rd_hdr_t;

typedef struct {
    uint8_t* base;
    uint32_t size;
} rdisk_t;

// errors
enum {
    RDISK_OK = 0,
    RDISK_E_BADARG = -1,
    RDISK_E_NOINIT = -2,
    RDISK_E_CORRUPT = -3,
    RDISK_E_NOFILE = -4,
    RDISK_E_EXISTS = -5,
    RDISK_E_NOSPACE = -6,
    RDISK_E_TOOMANY = -7,
    RDISK_E_TOOLONG = -8,
    RDISK_E_RANGE = -9,
};

#define RDISK_MAGIC 0x52444B31u // "RDK1"

static rd_hdr_t* hdr(rdisk_t* d) { return (rd_hdr_t*)d->base; }

static int streq(const char* a, const char* b) {
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return (*a == 0 && *b == 0);
}

static void strcopy0(char* dst, const char* src, int max) {
    int i = 0;
    for (; i < max - 1 && src[i]; i++) dst[i] = src[i];
    dst[i] = 0;
}

static int strlen_s(const char* s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

static int find_idx(rdisk_t* d, const char* name) {
    rd_hdr_t* h = hdr(d);
    for (uint32_t i = 0; i < RDISK_MAX_FILES; i++) {
        if ((h->files[i].flags & 1u) && streq(h->files[i].name, name)) return (int)i;
    }
    return -1;
}

static int find_free_entry(rdisk_t* d) {
    rd_hdr_t* h = hdr(d);
    for (uint32_t i = 0; i < RDISK_MAX_FILES; i++) {
        if ((h->files[i].flags & 1u) == 0) return (int)i;
    }
    return -1;
}

static int rdisk_init(rdisk_t* d, void* mem, uint32_t bytes) {
    if (!d || !mem || bytes < sizeof(rd_hdr_t) + 256) return RDISK_E_BADARG;
    d->base = (uint8_t*)mem;
    d->size = bytes;

    rd_hdr_t* h = hdr(d);
    if (h->magic != RDISK_MAGIC || h->disk_size != bytes) {
        // Not formatted
        return RDISK_E_NOINIT;
    }
    if (h->version != 1) return RDISK_E_CORRUPT;
    if (h->next_free > bytes) return RDISK_E_CORRUPT;
    return RDISK_OK;
}

static int rdisk_format(rdisk_t* d) {
    if (!d || !d->base || d->size < sizeof(rd_hdr_t) + 256) return RDISK_E_BADARG;

    // zero fill
    for (uint32_t i = 0; i < d->size; i++) d->base[i] = 0;

    rd_hdr_t* h = hdr(d);
    h->magic = RDISK_MAGIC;
    h->version = 1;
    h->disk_size = d->size;
    h->num_files = 0;
    h->next_free = (uint32_t)sizeof(rd_hdr_t);

    return RDISK_OK;
}

static int rdisk_list(rdisk_t* d, void (*cb)(const rd_file_t* f, void* user), void* user) {
    if (!d || !d->base) return RDISK_E_BADARG;
    rd_hdr_t* h = hdr(d);
    if (h->magic != RDISK_MAGIC) return RDISK_E_NOINIT;

    for (uint32_t i = 0; i < RDISK_MAX_FILES; i++) {
        if (h->files[i].flags & 1u) {
            if (cb) cb(&h->files[i], user);
        }
    }
    return RDISK_OK;
}

static int rdisk_create(rdisk_t* d, const char* name, uint32_t cap_bytes) {
    if (!d || !d->base || !name) return RDISK_E_BADARG;
    if (cap_bytes == 0) return RDISK_E_BADARG;

    rd_hdr_t* h = hdr(d);
    if (h->magic != RDISK_MAGIC) return RDISK_E_NOINIT;

    if (strlen_s(name) >= RDISK_NAME_MAX) return RDISK_E_TOOLONG;
    if (find_idx(d, name) >= 0) return RDISK_E_EXISTS;

    int idx = find_free_entry(d);
    if (idx < 0) return RDISK_E_TOOMANY;

    // bump allocate
    uint32_t off = h->next_free;
    uint32_t end = off + cap_bytes;
    if (end > d->size) return RDISK_E_NOSPACE;

    rd_file_t* f = &h->files[idx];
    strcopy0(f->name, name, RDISK_NAME_MAX);
    f->size = 0;
    f->cap = cap_bytes;
    f->data_off = off;
    f->flags = 1u;

    h->num_files++;
    h->next_free = end;
    return RDISK_OK;
}

static int rdisk_delete(rdisk_t* d, const char* name) {
    if (!d || !d->base || !name) return RDISK_E_BADARG;
    rd_hdr_t* h = hdr(d);
    if (h->magic != RDISK_MAGIC) return RDISK_E_NOINIT;

    int idx = find_idx(d, name);
    if (idx < 0) return RDISK_E_NOFILE;

    h->files[idx].flags = 0; // データは残るが見えなくなる
    h->num_files--;
    return RDISK_OK;
}

static int rdisk_write(rdisk_t* d, const char* name, const void* data, uint32_t len) {
    if (!d || !d->base || !name || (!data && len)) return RDISK_E_BADARG;
    rd_hdr_t* h = hdr(d);
    if (h->magic != RDISK_MAGIC) return RDISK_E_NOINIT;

    int idx = find_idx(d, name);
    if (idx < 0) return RDISK_E_NOFILE;

    rd_file_t* f = &h->files[idx];
    if (len > f->cap) return RDISK_E_RANGE;

    uint8_t* dst = d->base + f->data_off;
    const uint8_t* src = (const uint8_t*)data;
    for (uint32_t i = 0; i < len; i++) dst[i] = src[i];
    f->size = len;
    return RDISK_OK;
}

static int rdisk_read(rdisk_t* d, const char* name, void* out, uint32_t maxlen, uint32_t* out_len) {
    if (!d || !d->base || !name || !out) return RDISK_E_BADARG;
    rd_hdr_t* h = hdr(d);
    if (h->magic != RDISK_MAGIC) return RDISK_E_NOINIT;

    int idx = find_idx(d, name);
    if (idx < 0) return RDISK_E_NOFILE;

    rd_file_t* f = &h->files[idx];
    uint32_t n = (f->size < maxlen) ? f->size : maxlen;

    uint8_t* dst = (uint8_t*)out;
    uint8_t* src = d->base + f->data_off;
    for (uint32_t i = 0; i < n; i++) dst[i] = src[i];
    if (out_len) *out_len = n;
    return RDISK_OK;
}

static int rdisk_get_ptr(rdisk_t* d, const char* name, void** ptr, uint32_t* len) {
    if (!d || !d->base || !name || !ptr) return RDISK_E_BADARG;
    rd_hdr_t* h = hdr(d);
    if (h->magic != RDISK_MAGIC) return RDISK_E_NOINIT;

    int idx = find_idx(d, name);
    if (idx < 0) return RDISK_E_NOFILE;

    rd_file_t* f = &h->files[idx];
    *ptr = (void*)(d->base + f->data_off);
    if (len) *len = f->size;
    return RDISK_OK;
}

static uint8_t g_disk_mem[64 * 1024];
static rdisk_t g_disk;
static int32_t g_disk_inited = 0;

int32_t rdisk_cmd_format(int32_t argc, char **argv)
{
    int rc = rdisk_init(&g_disk, g_disk_mem, sizeof(g_disk_mem));
    rdisk_format(&g_disk);
    g_disk_inited = 1;
    return 0;
}

static void print_file(const rd_file_t* f, void* u){
    (void)u;
    printf("%s  size=%u cap=%u\n", f->name, f->size, f->cap);
}

int32_t rdisk_cmd_dir(int32_t argc, char **argv)
{
    if (!g_disk_inited) {
        printf("Disk not initialized. Please format first.\n");
        return -1;
    }
    rdisk_list(&g_disk, print_file, NULL);
    return 0;
}

int32_t rdisk_cmd_save(int32_t argc, char **argv)
{
    int len;
    if (argc != 3) {
        return -1;
    }

    if (!g_disk_inited) {
        printf("Disk not initialized. Please format first.\n");
        return -1;
    }

    len = strlen_s(argv[1]);
    if (len >= RDISK_NAME_MAX) {
        return -1;
    }

    rdisk_create(&g_disk, argv[1], 128);
    rdisk_write(&g_disk, argv[1], argv[2], (uint32_t)strlen_s(argv[2]));
    return 0;
}

int32_t rdisk_cmd_type(int32_t argc, char **argv)
{
    void* p; uint32_t n;

    int len;
    if (argc != 2) {
        return -1;
    }

    len = strlen_s(argv[1]);
    if (len >= RDISK_NAME_MAX) {
        printf("Filename too long.\n");
        return 0;
    }

    if (!g_disk_inited) {
        printf("Disk not initialized. Please format first.\n");
        return 0;
    }

    int rc = rdisk_get_ptr(&g_disk, argv[1], &p, &n);
    if (rc != RDISK_OK) {
        printf("File not found.\n");
        return 0;
    }
    uint32_t to = (n < 256) ? n : 256;
    for (uint32_t i = 0; i < to; i++) {
        putchar(((uint8_t*)p)[i]);
    }
    putchar('\n');
    return 0;
}


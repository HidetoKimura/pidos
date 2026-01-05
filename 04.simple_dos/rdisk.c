// Comments must be in English.
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include "pico/stdlib.h"

#include "rdisk.h"
#include "service_table.h"

static jmp_buf g_run_jmp;
static volatile int g_exit_code = 0;
static volatile int g_in_run = 0;

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

    h->files[idx].flags = 0; // mark unused
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

static int rdisk_get_ptr_cap(rdisk_t* d, const char* name, void** ptr, uint32_t* cap) {
    if (!d || !d->base || !name || !ptr || !cap) return RDISK_E_BADARG;
    rd_hdr_t* h = (rd_hdr_t*)d->base;
    if (h->magic != RDISK_MAGIC) return RDISK_E_NOINIT;

    int idx = find_idx(d, name);
    if (idx < 0) return RDISK_E_NOFILE;

    rd_file_t* f = &h->files[idx];
    *ptr = (void*)(d->base + f->data_off);
    *cap = f->cap;
    return RDISK_OK;
}

static int rdisk_set_size(rdisk_t* d, const char* name, uint32_t size) {
    if (!d || !d->base || !name) return RDISK_E_BADARG;
    rd_hdr_t* h = (rd_hdr_t*)d->base;
    if (h->magic != RDISK_MAGIC) return RDISK_E_NOINIT;

    int idx = find_idx(d, name);
    if (idx < 0) return RDISK_E_NOFILE;

    rd_file_t* f = &h->files[idx];
    if (size > f->cap) return RDISK_E_RANGE;
    f->size = size;
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
int32_t rdisk_cmd_delete(int32_t argc, char **argv)
{
    int len;
    if (argc != 2) {
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

    rdisk_delete(&g_disk, argv[1]);
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

/*
 * Intel HEX record:
 * :LLAAAATT[DD...]CC
 *  LL = data length (bytes)
 *  AAAA = address (16-bit)
 *  TT = record type
 *  DD = data bytes
 *  CC = checksum (two's complement of sum of all bytes from LL..DD)
 */

static int hex_nib(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static int hex_byte(const char* p) {
    int hi = hex_nib(p[0]);
    int lo = hex_nib(p[1]);
    if (hi < 0 || lo < 0) return -1;
    return (hi << 4) | lo;
}

// Optional: Used if you want to keep information for type04/type05
static uint32_t g_ext_linear_base = 0;   // (type 04) << 16
static uint32_t g_start_linear_addr = 0; // (type 05)

// line: HEX record line
// dst: Destination buffer
// io_len: Number of bytes written (offset from the beginning)
// cap: Capacity of dst (in bytes)
// Return value: Success: 1 (data record write successful)
//               EOF record: 99
//               Ignored record: 0
//               Error: Negative value (various error codes)
static int ihex_feed_line_to_buffer(const char* line,
                             uint8_t* dst,
                             uint32_t* io_len,
                             uint32_t cap)
{
    if (!line || !dst || !io_len) return -100;

    // Ignore empty lines
    if (line[0] == '\0') return 0;

    // First character ':'
    if (line[0] != ':') return -1;

    // Minimum length check: ":LLAAAATTCC" -> 1 + 2+4+2+2 = 13 chars
    // (Even with 0 data bytes, 13 characters are required)
    // Example: :00000001FF
    // In practice, the length of line is not measured, and a light check is done before accessing necessary parts
    // Here, only the minimum existence check is done (if too short, hex_byte returns -1)
    int ll = hex_byte(line + 1);
    int a1 = hex_byte(line + 3);
    int a2 = hex_byte(line + 5);
    int tt = hex_byte(line + 7);
    if (ll < 0 || a1 < 0 || a2 < 0 || tt < 0) return -2;
    if (ll > 255) return -3;

    const char* p = line + 9;

    // Checksum calculation
    uint32_t sum = (uint8_t)ll + (uint8_t)a1 + (uint8_t)a2 + (uint8_t)tt;

    // Read data section
    uint8_t data[256];
    for (int i = 0; i < ll; i++) {
        int b = hex_byte(p + i * 2);
        if (b < 0) return -4;
        data[i] = (uint8_t)b;
        sum += data[i];
    }
    p += ll * 2;

    // Checksum byte
    int cc = hex_byte(p);
    if (cc < 0) return -5;

    // sum + checksum lower 8 bits should be 0 (two's complement)
    if (((uint8_t)(sum + (uint8_t)cc)) != 0) return -6;

    // Record type specific processing
    switch (tt) {
    case 0x00: { // Data record
        // "Continuous storage" method: ignore address and append
        if (*io_len + (uint32_t)ll > cap) return -7; // overflow
        for (int i = 0; i < ll; i++) {
            dst[(*io_len)++] = data[i];
        }
        return 1;
    }

    case 0x01: // EOF
        return 99;

    case 0x04: { // Extended Linear Address (2 bytes)
        if (ll != 2) return -8;
        // Keep upper 16 bits (usually not used in this method, but for debugging)
        g_ext_linear_base = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16);
        return 0;
    }

    case 0x05: { // Start Linear Address (4 bytes)
        if (ll != 4) return -9;
        g_start_linear_addr = ((uint32_t)data[0] << 24) |
                              ((uint32_t)data[1] << 16) |
                              ((uint32_t)data[2] <<  8) |
                              ((uint32_t)data[3] <<  0);
        return 0;
    }

    default:
        // Other types (02/03 etc.) are ignored in the minimal implementation
        return 0;
    }
}

#define INPUT_BUFFER_SIZE 256

static int read_line(char* buf, int maxlen)
{
    char* input = buf;
    int index = 0;

    while (true) {
        int ch = getchar_timeout_us(0);  // Non-blocking read
        if (ch != PICO_ERROR_TIMEOUT) {
            if (ch == '\r' || ch == '\n') {
                input[index] = '\0';  // Null-terminate the string
                break;
            } else if (index < INPUT_BUFFER_SIZE - 1) {
                input[index++] = ch;
            }
        }
    }
    return index;
}

static uint32_t parse_hex_u32(const char* s){
    uint32_t v=0;
    while (*s){
        int n = -1;
        if ('0'<=*s && *s<='9') n=*s-'0';
        else if ('A'<=*s && *s<='F') n=*s-'A'+10;
        else if ('a'<=*s && *s<='f') n=*s-'a'+10;
        else break;
        v = (v<<4) | (uint32_t)n;
        s++;
    }
    return v;
}

// line exsample: "loadhex HELLO 400" (cap=0x400)
int32_t rdisk_cmd_loadhex(int32_t argc, char **argv)
{
    if (argc != 3){
        return -1;
    }
    uint32_t cap = parse_hex_u32(argv[2]);
    if (cap == 0){
        printf("caphex must be >0\n");
        return -1;
    }
    if (!g_disk_inited) {
        printf("Disk not initialized. Please format first.\n");
        return -1;
    }
    // create file
    if (rdisk_create(&g_disk, argv[1], cap) != RDISK_OK){
        printf("(create failed) maybe exists? try del first.\n");
        return -1;
    }

    #define IHEX_DATA_MAX 600

    void* ptr=0; 
    uint32_t real_cap=0;
    if (rdisk_get_ptr_cap(&g_disk, argv[1], &ptr, &real_cap) != RDISK_OK){
        printf("(get ptr failed)\n");
        return -1;
    }

    printf("[LOADHEX] send Intel HEX lines, end with EOF(:..01..)\n");

    uint8_t* dst = (uint8_t*)ptr;
    uint32_t len = 0;

    char ihex[IHEX_DATA_MAX];
    int r=0;
    printf("(IHEX)> ");
    while (1){
        int n = read_line(ihex, sizeof(ihex));
        if (n <= 0) continue;

        r = ihex_feed_line_to_buffer(ihex, dst, &len, real_cap);
        if (r == 99) break;
        if (r < 0){
            printf("LOADHEX ERROR: %d\n", r);
            break;
        }
    }
    printf("\n");

    rdisk_set_size(&g_disk, argv[1], len);

    printf("[SAVED] %s\n", argv[1]);
    printf("size = 0x%08X \n", len);

    return 0;
}

#define RUN_ADDR  (0x20020000u)

static void os_exit_impl(int code){
    g_exit_code = code;
    g_in_run = 0;
    longjmp(g_run_jmp, 1);      // ★Return to the caller here
}

static void service_table_init(void)
{
    svc_tbl()->puts = puts;
    svc_tbl()->exit = os_exit_impl;
}

static void mem_copy8(uint8_t* d, const uint8_t* s, uint32_t n)
{
    for (uint32_t i=0;i<n;i++) d[i]=s[i];
}

static int os_run_image(uint32_t load_addr)
{
    // Setup for longjmp return
    g_in_run = 1;
    g_exit_code = 0;

    if (setjmp(g_run_jmp) != 0) {
        // Returned via longjmp
        return g_exit_code;
    }

    // Thumb mode, so jump with bit0=1
    void (*entry)(void) = (void(*)(void))((load_addr | 1u));
    entry();

    // If entry returns (app that does not call exit)
    g_in_run = 0;
    return 0;
}

// "run NAME [ADDR]"
int32_t rdisk_cmd_run(int32_t argc, char **argv)
{
    if (argc < 2 || argc > 3){
        printf("Usage: run <name> [addrhex]\n");
        return -1;
    }
    if (!g_disk_inited) {
        printf("Disk not initialized. Please format first.\n");
        return -1;
    }

    const char* name = argv[1];
    uint32_t addr = (argc >= 3) ? parse_hex_u32(argv[2]) : (uint32_t)RUN_ADDR;

    void* p=0; 
    uint32_t len=0;

    if (rdisk_get_ptr(&g_disk, name, &p, &len) != RDISK_OK){
        printf("No such file\n");
        return -1;
    }
    if (len == 0){
        printf("Empty file\n");
        return -1;
    }

    // Initialize service table
    service_table_init();

    // Copy
    mem_copy8((uint8_t*)addr, (const uint8_t*)p, len);

    printf("[RUN] %s at 0x%08X\n", name, addr);

    int rc = os_run_image(addr);

    printf("[RETURN] rc = %d\n", rc);

    return 0;
}




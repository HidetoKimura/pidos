#include "fs/flash_fs.h"
#include "fs/ramfs.h"

#include <string.h>
#include <stdint.h>

#include "hardware/flash.h"
#include "pico/flash.h"
#include "pico/stdlib.h"


// ---- Adjust these as needed ----
#define FS_TOTAL_BYTES      (64u * 1024u)       // Total 64KB
#define FS_SLOT_BYTES       (FS_TOTAL_BYTES/2)  // 32KB each (A/B)

// Reserve at end of flash
#define FS_FLASH_BASE_OFFSET (PICO_FLASH_SIZE_BYTES - FS_TOTAL_BYTES)
#define FS_SLOT0_OFFSET      (FS_FLASH_BASE_OFFSET + 0)
#define FS_SLOT1_OFFSET      (FS_FLASH_BASE_OFFSET + FS_SLOT_BYTES)
// --------------------

#define FS_MAGIC 0x50444F53u  // 'PDOS'

typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t size;   // payload size
    uint32_t crc;    // payload crc
} fs_hdr_t;

static uint32_t crc32_simple(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t*)data;
    uint32_t x = 0x12345678u;
    for (size_t i = 0; i < len; i++) x = (x * 33u) ^ p[i];
    return x;
}

static const uint8_t* flash_ptr(uint32_t off) {
    return (const uint8_t*)(XIP_BASE + off);
}

static bool hdr_valid(const fs_hdr_t *h) {
    if (h->magic != FS_MAGIC) return false;
    if (h->size == 0) return false;
    if (h->size > (FS_SLOT_BYTES - sizeof(fs_hdr_t))) return false;
    return true;
}

static bool read_slot(uint32_t slot_off, fs_hdr_t *hdr_out) {
    const fs_hdr_t *h = (const fs_hdr_t*)flash_ptr(slot_off);
    *hdr_out = *h;
    if (!hdr_valid(hdr_out)) return false;

    const uint8_t *payload = flash_ptr(slot_off + sizeof(fs_hdr_t));
    uint32_t crc = crc32_simple(payload, hdr_out->size);
    return crc == hdr_out->crc;
}

typedef struct {
    uint32_t offset;
    const uint8_t *src;
    size_t len; // MUST be FS_SLOT_BYTES
} prog_args_t;

// Run from RAM since XIP halts during flash writes
static void __not_in_flash_func(do_erase_prog)(void *p) {
    prog_args_t *a = (prog_args_t*)p;

    // Erase entire slot (4KB sectors)
    flash_range_erase(a->offset, a->len);

    // Program in 256B pages
    for (size_t i = 0; i < a->len; i += FLASH_PAGE_SIZE) {
        flash_range_program(a->offset + (uint32_t)i, a->src + i, FLASH_PAGE_SIZE);
    }
}

bool flash_fs_load(void) {
    fs_hdr_t h0, h1;
    bool v0 = read_slot(FS_SLOT0_OFFSET, &h0);
    bool v1 = read_slot(FS_SLOT1_OFFSET, &h1);

    if (!v0 && !v1) return false;

    uint32_t best_off;
    fs_hdr_t best;
    if (v0 && (!v1 || h0.seq >= h1.seq)) { best_off = FS_SLOT0_OFFSET; best = h0; }
    else { best_off = FS_SLOT1_OFFSET; best = h1; }

    const uint8_t *payload = flash_ptr(best_off + sizeof(fs_hdr_t));
    return ramfs_deserialize(payload, best.size);
}

bool flash_fs_save(void) {
    fs_hdr_t h0, h1;
    bool v0 = read_slot(FS_SLOT0_OFFSET, &h0);
    bool v1 = read_slot(FS_SLOT1_OFFSET, &h1);

    uint32_t cur_seq = 0;
    uint32_t dst_off = FS_SLOT0_OFFSET;

    if (v0 && (!v1 || h0.seq >= h1.seq)) { cur_seq = h0.seq; dst_off = FS_SLOT1_OFFSET; }
    else if (v1) { cur_seq = h1.seq; dst_off = FS_SLOT0_OFFSET; }

    static uint8_t slot_buf[FS_SLOT_BYTES];
    memset(slot_buf, 0xFF, sizeof(slot_buf));

    fs_hdr_t *hdr = (fs_hdr_t*)slot_buf;
    uint8_t *payload = slot_buf + sizeof(fs_hdr_t);
    size_t payload_cap = FS_SLOT_BYTES - sizeof(fs_hdr_t);

    size_t sz = ramfs_serialize(payload, payload_cap);
    if (sz == 0) return false;

    hdr->magic = FS_MAGIC;
    hdr->seq   = cur_seq + 1;
    hdr->size  = (uint32_t)sz;
    hdr->crc   = crc32_simple(payload, sz);

    prog_args_t args = { .offset = dst_off, .src = slot_buf, .len = FS_SLOT_BYTES };

    // Execute safely with IRQ/multicore coordination
    int rc = flash_safe_execute(do_erase_prog, &args, 2000);
    return rc == PICO_OK;
}

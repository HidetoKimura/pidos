// pxe_loader.c (OS側)
#include "pxe_format.h"
#include "vfs/vfs.h"
#include <string.h>
#include <stdint.h>

#define APP_BASE ((uint8_t*)0x20020000u)
#define APP_SIZE (64u*1024u)

typedef int (*pxe_entry_t)(int argc, char** argv);

static bool read_exact(int fd, void* buf, uint32_t n) {
    vfs_err_t e;
    uint8_t* p = (uint8_t*)buf;
    uint32_t got = 0;
    while (got < n) {
        int r = vfs_read(fd, p + got, n - got, &e);
        if (r <= 0) return false;
        got += (uint32_t)r;
    }
    return true;
}

bool pxe_run_fixed(const char* path, int argc, char** argv) {
    vfs_err_t e;
    int fd = vfs_open(path, VFS_O_RDONLY, &e);
    if (fd < 0) return false;

    pxe_hdr_t h;
    if (!read_exact(fd, &h, sizeof(h))) { vfs_close(fd); return false; }
    if (h.magic != PXE_MAGIC || h.ver != PXE_VER) { vfs_close(fd); return false; }

    uint32_t need = h.image_size + h.bss_size;
    if (need > APP_SIZE) { vfs_close(fd); return false; }

    if (!read_exact(fd, APP_BASE, h.image_size)) { vfs_close(fd); return false; }
    vfs_close(fd);

    if (h.bss_size) memset(APP_BASE + h.image_size, 0, h.bss_size);

    pxe_entry_t entry = (pxe_entry_t)(APP_BASE + h.entry_off);
    int rc = entry(argc, argv);
    (void)rc;
    return true;
}

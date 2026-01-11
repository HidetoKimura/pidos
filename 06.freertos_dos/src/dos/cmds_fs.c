// cmds_fs.c
#include "dos/cmds_fs.h"
#include "dos/dos_sys.h"
#include "vfs/vfs.h"
#include "fs/ramfs.h"
#include <string.h>

static void cmd_dir(void) {
    dos_puts(" Directory of A:\\\r\n\r\n");
    for (int i=0;i<16;i++){
        char name[16];
        size_t sz=0;
        bool used=false;
        if (!ramfs_list(i, name, sizeof(name), &sz, &used)) continue;
        if (!used) continue;
        dos_printf(" %8u  %s\r\n", (unsigned)sz, name);
    }
    dos_puts("\r\n");
}

static void cmd_type(const char* path) {
    vfs_err_t e;
    int fd = vfs_open(path, VFS_O_RDONLY, &e);
    if (fd < 0) { dos_puts("File not found.\r\n"); return; }

    char buf[64];
    while (1) {
        int n = vfs_read(fd, buf, sizeof(buf), &e);
        if (n <= 0) break;
        vfs_write(1, buf, (size_t)n, &e);
    }
    vfs_close(fd);
    dos_puts("\r\n");
}

static void cmd_copy(const char* src, const char* dst) {
    vfs_err_t e;
    int fds = vfs_open(src, VFS_O_RDONLY, &e);
    if (fds < 0) { dos_puts("Source not found.\r\n"); return; }

    int fdd = vfs_open(dst, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC, &e);
    if (fdd < 0) { vfs_close(fds); dos_puts("Cannot create dest.\r\n"); return; }

    char buf[64];
    while (1) {
        int n = vfs_read(fds, buf, sizeof(buf), &e);
        if (n <= 0) break;
        if (vfs_write(fdd, buf, (size_t)n, &e) < 0) { dos_puts("Write error.\r\n"); break; }
    }
    vfs_close(fds);
    vfs_close(fdd);
    dos_puts("1 file(s) copied.\r\n");
}

static void cmd_del(const char* path) {
    vfs_err_t e;
    if (!ramfs_delete(path, &e)) {
        dos_puts("File not found.\r\n");
        return;
    }
    dos_puts("Deleted.\r\n");
}

bool cmds_fs_try(int argc, char** argv) {
    if (strcmp(argv[0], "DIR") == 0) { cmd_dir(); return true; }
    if (strcmp(argv[0], "TYPE") == 0) {
        if (argc < 2) { dos_puts("Usage: TYPE <file>\r\n"); return true; }
        cmd_type(argv[1]); return true;
    }
    if (strcmp(argv[0], "COPY") == 0) {
        if (argc < 3) { dos_puts("Usage: COPY <src> <dst>\r\n"); return true; }
        cmd_copy(argv[1], argv[2]); return true;
    }
    if (strcmp(argv[0], "DEL") == 0) {
        if (argc < 2) { dos_puts("Usage: DEL <file>\r\n"); return true; }
        cmd_del(argv[1]); return true;
    }
    return false;
}

// cmds_fs.c
#include "dos/cmds_fs.h"
#include "dos/dos_sys.h"
#include "dos/dos.h"
#include "vfs/vfs.h"
#include "fs/ramfs.h"
#include <string.h>

// cmds_fs.c（DIR部分差し替え + 追記）

static void cmd_dir(const char* path_or_null) {
    vfs_err_t e;
    char pwd[96];
    // 表示は現在のcwdでよい（path指定のときも簡単化したければ省略可）
    ramfs_pwd(pwd, sizeof(pwd));
    dos_printf(" Directory of %s\r\n\r\n", pwd);

    for (int i=0;i<64;i++){
        ramfs_dirent_t de;
        if (!ramfs_list_dir(path_or_null, i, &de, &e)) { dos_puts("DIR error.\r\n"); return; }
        if (!de.used) break;

        if (de.is_dir) dos_printf(" <DIR>     %s\r\n", de.name);
        else           dos_printf(" %8u  %s\r\n", (unsigned)de.size, de.name);
    }
    dos_puts("\r\n");
}

static void cmd_cd(const char* path) {
    vfs_err_t e;
    if (!ramfs_cd(path, &e)) { dos_puts("The system cannot find the path specified.\r\n"); return; }
}

static void cmd_md(const char* path) {
    vfs_err_t e;
    if (!ramfs_mkdir(path, &e)) { dos_puts("Cannot create directory.\r\n"); return; }
}

static void cmd_rd(const char* path) {
    vfs_err_t e;
    if (!ramfs_rmdir(path, &e)) {
        if (e == VFS_E_BUSY) dos_puts("Directory not empty.\r\n");
        else dos_puts("Cannot remove directory.\r\n");
        return;
    }
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
    if (strcmp(argv[0], "DIR") == 0) {
        cmd_dir(argc >= 2 ? argv[1] : NULL);
        return true;
    }
    if (strcmp(argv[0], "CD") == 0) {
        if (argc < 2) {
            char pwd[96]; ramfs_pwd(pwd, sizeof(pwd));
            dos_printf("%s\r\n", pwd);
        } else cmd_cd(argv[1]);
        return true;
    }
    if (strcmp(argv[0], "MD") == 0 || strcmp(argv[0], "MKDIR") == 0) {
        if (argc < 2) { dos_puts("Usage: MD <dir>\r\n"); return true; }
        cmd_md(argv[1]); return true;
    }
    if (strcmp(argv[0], "RD") == 0 || strcmp(argv[0], "RMDIR") == 0) {
        if (argc < 2) { dos_puts("Usage: RD <dir>\r\n"); return true; }
        cmd_rd(argv[1]); return true;
    }

    // TYPE/COPY/DELはそのまま動く（ramfs_open/deleteがパス対応したので）
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

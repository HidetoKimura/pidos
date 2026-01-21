// cmds_core.c
#include <string.h>
#include "dos/cmds_core.h"
#include "dos/dos_sys.h"
#include "dos/apps_builtin.h"
#include "vfs/vfs.h"
#include "fs/flash_fs.h"
#include "fs/ramfs.h"
#include "pxe/pxe_loader.h"
#include "xfer/xfer_recv.h"

static void cmd_help(void) {
    dos_puts(
        "Commands:\r\n"
        "  HELP\r\n"
        "  ECHO [text]\r\n"
        "  CLS\r\n"
        "  RUN <app> [args]\r\n"
        "  DIR\r\n"
        "  TYPE <file>\r\n"
        "  COPY <src> <dst>\r\n"
        "  DEL <file>\r\n"
        "  MD <dir>\r\n"
        "  CD <dir>\r\n"
        "  RD <dir>\r\n"
        "  SAVE\r\n"
        "  LOAD\r\n"
        "  RECV <file>\r\n"
    );
}

static void echo_to_fd(int fd, int argc, char** argv, int from, int to) {
    vfs_err_t e;
    for (int i=from;i<to;i++){
        const char* s = argv[i];
        vfs_write(fd, s, strlen(s), &e);
        if (i != to-1) vfs_write(fd, " ", 1, &e);
    }
    vfs_write(fd, "\n", 1, &e); // vfs converts to CRLF
}

static bool echo_main(int argc, char** argv) {

    int redir = -1;
    bool append = false;
    for (int i=1;i<argc;i++){
        if (strcmp(argv[i], ">") == 0) { redir = i; append = false; break; }
        if (strcmp(argv[i], ">>") == 0){ redir = i; append = true; break; }
    }

    if (redir < 0) {
        echo_to_fd(1, argc, argv, 1, argc);
        return true;
    }

    if (redir + 1 >= argc) { dos_puts("Syntax error.\r\n"); return true; }

    const char* outpath = argv[redir + 1];
    vfs_err_t e;
    int mode = VFS_O_WRONLY | VFS_O_CREAT | (append ? VFS_O_APPEND : VFS_O_TRUNC);
    int fd = vfs_open(outpath, mode, &e);
    if (fd < 0) { dos_puts("Cannot open output file.\r\n"); return true; }

    echo_to_fd(fd, argc, argv, 1, redir);
    vfs_close(fd);
    return true;
}

static void cmd_cls(void) {
    // ANSI clear
    dos_puts("\x1b[2J\x1b[H");
}

static void cmd_save(void) {
    if (!ramfs_is_dirty()) {
        dos_puts("No changes.\r\n");
        return;
    }
    dos_puts("Saving...\r\n");
    if (flash_fs_save()) {
        ramfs_clear_dirty();
        dos_puts("Saved.\r\n");
    } else {
        dos_puts("Save failed.\r\n");
    }
}

static void cmd_load(void) {
    dos_puts("Loading...\r\n");
    if (flash_fs_load()) {
        // State is valid after load, so clear dirty
        ramfs_clear_dirty();
        dos_puts("Loaded.\r\n");
    } else {
        dos_puts("Load failed (no valid image?).\r\n");
    }
}


#define PATH_MAX 64
static bool cmd_run_pxe(int argc, char** argv) {

    if (argc < 2) { dos_puts("Usage: RUN <app>\r\n"); return true; }

    // First, try builtin apps by name
    if (apps_builtin_run(argv[1], argc-1, &argv[1])) return true;

    // Example: append .PXE if no extension
    char path[PATH_MAX];
    strncpy(path, argv[1], sizeof(path)-1);
    path[sizeof(path)-1] = 0;
    // Simplest: append .PXE at end (skip if already present; PATH search later)
    if (!strstr(path, ".PXE")) strncat(path, ".PXE", sizeof(path)-strlen(path)-1);

    if (!pxe_run_fixed(path, argc-1, &argv[1])) {
        dos_puts("PXE load/run failed.\r\n");
    }
    return true;
}


bool cmds_core_try(int argc, char** argv) {
    if (strcmp(argv[0], "HELP") == 0) {
        cmd_help(); return true;
    }
    if (strcmp(argv[0], "CLS") == 0) {
        cmd_cls(); return true;
    }
    if (strcmp(argv[0], "ECHO") == 0) {
        bool rc = echo_main(argc, argv);
        return rc;
    }
    if (strcmp(argv[0], "RUN") == 0) {
        bool rc = cmd_run_pxe(argc, argv);
        return rc;
#if 0
        if (argc < 2) { dos_puts("Usage: RUN <app> [args]\r\n"); return true; }
        if (!apps_run(argv[1], argc-1, &argv[1])) {
            dos_puts("App not found.\r\n");
        }
        return true;
#endif
        }
    if (strcmp(argv[0], "SAVE") == 0) {
        cmd_save();
        return true;
    }
    if (strcmp(argv[0], "LOAD") == 0) {
        cmd_load();
        return true;
    }

    if (strcmp(argv[0], "RECV") == 0) {
        if (argc < 2) { dos_puts("Usage: RECV <path>\r\n"); return true; }
        if (!xfer_recv_file(argv[1])) dos_puts("RECV failed\r\n");
        return true;
    }

    return false;
}

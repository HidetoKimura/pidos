// cmds_core.c
#include "dos/cmds_core.h"
#include "dos/dos_sys.h"
#include "dos/apps.h"
#include "vfs/vfs.h"
#include <string.h>

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
    );
}

static void echo_to_fd(int fd, int argc, char** argv, int from, int to) {
    vfs_err_t e;
    for (int i=from;i<to;i++){
        const char* s = argv[i];
        vfs_write(fd, s, strlen(s), &e);
        if (i != to-1) vfs_write(fd, " ", 1, &e);
    }
    vfs_write(fd, "\n", 1, &e); // vfs側がCRLFにしてくれる
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
        if (argc < 2) { dos_puts("Usage: RUN <app> [args]\r\n"); return true; }
        if (!apps_run(argv[1], argc-1, &argv[1])) {
            dos_puts("App not found.\r\n");
        }
        return true;
    }
    return false;
}

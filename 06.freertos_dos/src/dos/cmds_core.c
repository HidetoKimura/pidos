// cmds_core.c
#include "dos/cmds_core.h"
#include "dos/dos_sys.h"
#include "dos/apps.h"
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
        for (int i=1;i<argc;i++){
            dos_puts(argv[i]);
            if (i != argc-1) dos_putc(' ');
        }
        dos_puts("\r\n");
        return true;
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

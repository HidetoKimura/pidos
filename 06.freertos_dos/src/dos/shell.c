// shell.c
#include "dos/shell.h"
#include "dos/dos.h"
#include "dos/dos_sys.h"
#include "dos/parse.h"
#include "dos/cmds_core.h"
#include "dos/cmds_fs.h"
#include "util/strutil.h"
#include <string.h>
#include <stdbool.h>

static void read_line(char* out, int cap) {
    int n = 0;
    while (n < cap-1) {
        int c = dos_getc_blocking();
        if (c == '\r' || c == '\n') {
            dos_putc('\r'); dos_putc('\n');
            break;
        }
        if (c == 0x7f || c == 0x08) { // backspace
            if (n > 0) { n--; dos_puts("\b \b"); }
            continue;
        }
        if (c < 0x20) continue;
        out[n++] = (char)c;
        dos_putc((char)c);
    }
    out[n] = '\0';
}

void shell_run(void) {
    char line[DOS_MAX_LINE];

    while (1) {
        dos_puts("A:\\> ");

        read_line(line, sizeof(line));
        if (!line[0]) continue;

        dos_argv_t av;
        if (!dos_parse_line(line, &av)) continue;

        // コマンドは大文字扱い（DOSっぽく）
        str_to_upper(av.argv[0]);

        if (cmds_core_try(av.argc, av.argv)) continue;
        if (cmds_fs_try(av.argc, av.argv)) continue;

        dos_printf("Bad command or file name: %s\n", av.argv[0]);
    }
}

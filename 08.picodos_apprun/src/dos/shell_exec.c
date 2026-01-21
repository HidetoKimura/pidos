#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "dos/dos.h"
#include "dos/dos_sys.h"
#include "dos/cmds_core.h"
#include "dos/cmds_fs.h"

static bool is_space(char c) { return c==' ' || c=='\t'; }

/*
 * Minimal tokenizer:
 * - Split on spaces/tabs
 * - Support "..." double quotes (preserve inner spaces)
 * - No escapes (minimal)
 */
static int split_argv(char* buf, char** argv, int argv_max) {
    int argc = 0;
    char* p = buf;

    while (*p) {
        while (is_space(*p)) p++;
        if (*p == 0) break;
        if (argc >= argv_max) break;

        if (*p == '"') {
            // quoted token
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') { *p = 0; p++; }
        } else {
            argv[argc++] = p;
            while (*p && !is_space(*p)) p++;
            if (*p) { *p = 0; p++; }
        }
    }
    return argc;
}

/*
 * Execute one line: usable from AUTOEXEC and interactive input
 */
void shell_execute_line(const char* line) {
    if (!line) return;

    // Copy to buffer and normalize to tolerate trailing newline etc.
    char buf[160];
    size_t n = strnlen(line, sizeof(buf) - 1);
    memcpy(buf, line, n);
    buf[n] = 0;

    // Skip leading whitespace
    char* s = buf;
    while (is_space(*s)) s++;
    if (*s == 0) return;

    // Do not execute lines starting with "REM ..." (redundant safety with AUTOEXEC)
    if ((s[0]=='R'||s[0]=='r') && (s[1]=='E'||s[1]=='e') && (s[2]=='M'||s[2]=='m') &&
        (s[3]==0 || is_space(s[3]))) {
        return;
    }

    // Split into argv
    char* argv[16];
    int argc = split_argv(s, argv, (int)(sizeof(argv)/sizeof(argv[0])));

    if (argc <= 0) return;

    if (cmds_core_try(argc, argv)) return;
    if (cmds_fs_try(argc, argv)) return;

    dos_puts("Bad command or file name.\r\n");
}

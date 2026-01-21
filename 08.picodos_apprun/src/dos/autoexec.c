#include "vfs/vfs.h"
#include "dos/dos.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "dos/dos_sys.h"

// Existing: function to execute a single command line (rename as suits your implementation)
extern void shell_execute_line(const char* line);

// Skip leading whitespace
static const char* lskip(const char* s){
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static bool starts_with_rem(const char* s){
    // Allow REM / rem / Rem (simple)
    if ((s[0]=='R'||s[0]=='r') && (s[1]=='E'||s[1]=='e') && (s[2]=='M'||s[2]=='m')) {
        char c = s[3];
        return (c==0 || c==' ' || c=='\t');
    }
    return false;
}

void dos_run_autoexec(void){
    vfs_err_t e;
    int fd = vfs_open("A:\\AUTOEXEC.BAT", VFS_O_RDONLY, &e);
    if (fd < 0) return; // Do nothing if not present

    dos_puts("[AUTOEXEC]\r\n");

    // Small is enough (line buffer)
    char line[128];
    int  li = 0;

    // Read buffer
    uint8_t buf[64];

    while (1) {
        int r = vfs_read(fd, buf, sizeof(buf), &e);
        if (r <= 0) break;

        for (int i=0; i<r; i++) {
            char c = (char)buf[i];

            if (c == '\r') continue;

            if (c == '\n') {
                line[li] = 0;
                li = 0;

                const char* s = lskip(line);
                if (*s == 0) continue;
                if (starts_with_rem(s)) continue;

                // Execute (optionally echo here)
                shell_execute_line(s);
                continue;
            }

            if (li < (int)sizeof(line)-1) {
                line[li++] = c;
            } else {
                // If line is too long: truncate tail (minimal implementation)
            }
        }
    }

    // If the last line lacked a trailing newline
    if (li > 0) {
        line[li] = 0;
        const char* s = lskip(line);
        if (*s && !starts_with_rem(s)) shell_execute_line(s);
    }

    vfs_close(fd);
    dos_puts("[AUTOEXEC END]\r\n");
}

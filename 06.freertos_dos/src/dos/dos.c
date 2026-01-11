#include "dos/dos.h"
#include "dos/dos_sys.h"
#include "dos/shell.h"
#include "dos/apps.h"
#include <stdarg.h>

void dos_init(void) {
    dos_sys_init();
    apps_init();
}

void dos_run(void) {
    shell_run();
}

void dos_print(const char* s) { dos_puts(s); }
void dos_println(const char* s) { dos_puts(s); dos_puts("\r\n"); }
void dos_printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    dos_vprintf(fmt, ap);
    va_end(ap);
}

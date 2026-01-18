// dos_sys.c
#include "dos_sys.h"
#include <stdio.h>
#include <stdarg.h>
#include "pico/stdlib.h"
#include "pico/stdio.h"

void dos_sys_init(void) {}

int dos_getc_blocking(void) {
    int c;
    do { c = getchar_timeout_us(0); } while (c == PICO_ERROR_TIMEOUT);
    return c;
}

void dos_putc(char c) { putchar_raw(c); }

void dos_puts(const char* s) {
    while (*s) dos_putc(*s++);
}

void dos_vprintf(const char* fmt, va_list ap) {
    vprintf(fmt, ap);
}


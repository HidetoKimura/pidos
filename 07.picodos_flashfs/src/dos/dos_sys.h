// dos_sys.h
#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

void dos_sys_init(void);
int  dos_getc_blocking(void);
void dos_putc(char c);
void dos_puts(const char* s);
void dos_vprintf(const char* fmt, va_list ap);
// high-level `dos_printf` is provided by dos.h/dos.c

#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void dos_init(void);
void dos_run(void);

void dos_print(const char* s);
void dos_println(const char* s);
void dos_printf(const char* fmt, ...);

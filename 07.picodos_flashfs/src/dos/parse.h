// parse.h
#pragma once
#include <stdbool.h>
#include <stddef.h>

#define DOS_MAX_ARGS 8
#define DOS_MAX_LINE 128

typedef struct {
    int argc;
    char* argv[DOS_MAX_ARGS];
} dos_argv_t;

bool dos_parse_line(char* line, dos_argv_t* out);

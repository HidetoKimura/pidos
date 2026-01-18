// parse.c
#include "parse.h"
#include <ctype.h>

static void skip_ws(char** p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

bool dos_parse_line(char* line, dos_argv_t* out) {
    out->argc = 0;
    for (int i = 0; i < DOS_MAX_ARGS; i++) out->argv[i] = 0;

    char* p = line;
    skip_ws(&p);
    if (!*p) return false;

    while (*p && out->argc < DOS_MAX_ARGS) {
        skip_ws(&p);
        if (!*p) break;

        out->argv[out->argc++] = p;

        // token ends at ws
        while (*p && !isspace((unsigned char)*p)) p++;
        if (*p) { *p = '\0'; p++; }
    }
    return out->argc > 0;
}

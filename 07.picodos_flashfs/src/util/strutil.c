// strutil.c
#include "strutil.h"
#include <ctype.h>

void str_to_upper(char* s) {
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

bool str_eq_nocase(const char* a, const char* b) {
    while (*a && *b) {
        char ca = (char)toupper((unsigned char)*a++);
        char cb = (char)toupper((unsigned char)*b++);
        if (ca != cb) return false;
    }
    return *a == *b;
}

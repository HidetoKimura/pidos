#include "util.h"

uint32_t parse_hex_u32(const char* s)
{
    uint32_t v=0;
    while (*s){
        int n = -1;
        if ('0'<=*s && *s<='9') n=*s-'0';
        else if ('A'<=*s && *s<='F') n=*s-'A'+10;
        else if ('a'<=*s && *s<='f') n=*s-'a'+10;
        else break;
        v = (v<<4) | (uint32_t)n;
        s++;
    }
    return v;
}
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

typedef int32_t (*command_t)(int32_t argc, char *argv[]);

typedef struct {
    command_t   command_func;
    const char* command_name;
    const char* usage;
} CommandList;

extern CommandList commands[];


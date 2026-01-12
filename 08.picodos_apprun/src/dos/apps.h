// apps.h
#pragma once
#include <stdbool.h>

typedef int (*app_main_t)(int argc, char** argv);

void apps_init(void);
bool apps_run(const char* name, int argc, char** argv);

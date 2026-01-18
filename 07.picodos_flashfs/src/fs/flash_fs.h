#pragma once
#include <stdbool.h>

bool flash_fs_load(void);  // Flash -> RAMFS
bool flash_fs_save(void);  // RAMFS -> Flash

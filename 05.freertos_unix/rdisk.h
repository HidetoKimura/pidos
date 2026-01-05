#pragma once
#include <stdint.h>
#include <stddef.h>

int32_t rdisk_cmd_format(int32_t argc, char **argv);
int32_t rdisk_cmd_dir(int32_t argc, char **argv);
int32_t rdisk_cmd_save(int32_t argc, char **argv);
int32_t rdisk_cmd_delete(int32_t argc, char **argv);
int32_t rdisk_cmd_type(int32_t argc, char **argv);
int32_t rdisk_cmd_type(int32_t argc, char **argv);
int32_t rdisk_cmd_loadhex(int32_t argc, char **argv);
int32_t rdisk_cmd_run(int32_t argc, char **argv);
int32_t rdisk_read_file(const char* name, void** ptr, uint32_t* len);
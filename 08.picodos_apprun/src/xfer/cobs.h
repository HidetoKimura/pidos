#pragma once
#include <stddef.h>
#include <stdint.h>

// returns decoded length, 0 on error
size_t cobs_decode(const uint8_t* in, size_t in_len, uint8_t* out, size_t out_cap);

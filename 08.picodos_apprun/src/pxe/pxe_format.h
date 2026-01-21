// pxe_format.h
#pragma once
#include <stdint.h>

#define PXE_MAGIC 0x30584550u   // 'PXE0'
#define PXE_VER   1

typedef struct {
    uint32_t magic;
    uint16_t ver;
    uint16_t flags;      // 0
    uint32_t image_size; // raw binary size
    uint32_t bss_size;   // bytes (optional; 0 is OK)
    uint32_t entry_off;  // entry offset from load base
    uint32_t reserved[3];
} pxe_hdr_t;

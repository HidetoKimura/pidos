#pragma once
#include <stdint.h>

typedef struct {
    int (*puts)(const char*);
    void (*exit)(int code);
} svc_tbl_t;

#define SVC_TBL_ADDR 0x20030000u   // Example: placed at a safe fixed RAM location (adjust as needed)
static inline volatile svc_tbl_t* svc_tbl(void){
    return (volatile svc_tbl_t*)SVC_TBL_ADDR;
}

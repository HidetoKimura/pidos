#include <stdint.h>
typedef struct {
    int (*puts)(const char*);
    void (*exit)(int code);
} svc_tbl_t;

#define SVC_TBL_ADDR 0x20030000u   // 例：安全なRAM固定位置に置く（要調整）
static inline volatile svc_tbl_t* svc_tbl(void){
    return (volatile svc_tbl_t*)SVC_TBL_ADDR;
}

void main(void){
    svc_tbl()->puts("HELLO\n");
    svc_tbl()->exit(123);  // ★OSへ戻る
    while(1){}
}

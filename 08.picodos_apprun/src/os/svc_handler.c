#include <stdint.h>
#include "vfs/vfs.h"

// 例外フレーム（Cortex-M0+ が自動で積むやつ）
typedef struct {
    uint32_t r0,r1,r2,r3,r12,lr,pc,xpsr;
} exc_frame_t;

// --- syscall番号は r12 を使う設計（アプリ側も r12 に入れて svc） ---
static int k_write(int fd, const void* buf, int len) {
    vfs_err_t e;
    if (len < 0) return -1;
    return vfs_write(fd, buf, (size_t)len, &e);
}

static int syscall_dispatch(int no, int a0, int a1, int a2, int a3) {
    (void)a3;
    switch (no) {
    case 2: return k_write(a0, (const void*)a1, a2); // SYS_write
    case 1: return 0;                                // SYS_exit（最短版：戻り値だけ）
    default: return -1;
    }
}

// naked: 例外フレームのポインタをCへ渡す
__attribute__((naked)) void SVC_Handler(void) {
    __asm volatile(
        "mov r0, sp\n"
        "b   SVC_Handler_C\n"
    );
}

void SVC_Handler_C(exc_frame_t* f) {
    int no = (int)f->r12;
    int ret = syscall_dispatch(no, (int)f->r0, (int)f->r1, (int)f->r2, (int)f->r3);
    f->r0 = (uint32_t)ret; // syscall戻り値
}

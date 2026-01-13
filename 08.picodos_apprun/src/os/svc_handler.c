#include <stdint.h>
#include "vfs/vfs.h"
#include "os/syscall.h"

// Cortex-M0+ exception frame
typedef struct {
    uint32_t r0,r1,r2,r3,r12,lr,pc,xpsr;
} exc_frame_t;

static int k_write(int fd, const void* buf, int len) {
    vfs_err_t e;
    if (len < 0) return -1;
    return vfs_write(fd, buf, (size_t)len, &e);
}

static int k_open(const char* path, int flags) {
    vfs_err_t e;
    return vfs_open(path, flags, &e);
}
static int k_close(int fd) {
    return vfs_close(fd);
}

static int syscall_dispatch(int no, int a0, int a1, int a2, int a3) {
    (void)a3;
    switch (no) {
    case SYS_write: return k_write(a0, (const void*)a1, a2);
    case SYS_open:  return k_open((const char*)a0, a1);
    case SYS_close: return k_close(a0);
    case SYS_exit:  return 0;
    default: return -1;
    }
}

// ★ Pico SDK が呼ぶのは isr_svcall
__attribute__((naked)) void isr_svcall(void) {
    __asm volatile(
        "mov r0, sp\n"
        "b   isr_svcall_c\n"
    );
}

void isr_svcall_c(exc_frame_t* f) {
    int no  = (int)f->r12; // syscall番号はr12に入れる設計
    int ret = syscall_dispatch(no, (int)f->r0, (int)f->r1, (int)f->r2, (int)f->r3);
    f->r0 = (uint32_t)ret;
}

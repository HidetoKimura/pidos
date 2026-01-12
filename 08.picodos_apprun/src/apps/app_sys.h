// app_sys.h (アプリ側)
#pragma once
#include <stdint.h>

enum {
    SYS_exit  = 1,
    SYS_write = 2,
};

static inline int sys_call(int no, int a0, int a1, int a2, int a3) {
    register int r0  __asm("r0")  = a0;
    register int r1  __asm("r1")  = a1;
    register int r2  __asm("r2")  = a2;
    register int r3  __asm("r3")  = a3;
    register int r12 __asm("r12") = no;
    __asm volatile ("svc 0"
                    : "+r"(r0)
                    : "r"(r1), "r"(r2), "r"(r3), "r"(r12)
                    : "memory");
    return r0;
}

static inline int sys_write(int fd, const void* buf, int len) {
    return sys_call(SYS_write, fd, (int)buf, len, 0);
}
static inline void sys_exit(int code) {
    sys_call(SYS_exit, code, 0, 0, 0);
    while (1) {}
}

// app_hello.c
#include "app_sys.h"

__attribute__((section(".text.app_entry")))
int app_entry(int argc, char** argv) {
    (void)argc; (void)argv;
    const char msg[] = "Hello from PXE app via SVC!\n";
    sys_write(1, msg, (int)(sizeof(msg)-1));
    sys_exit(0);
    return 0;
}

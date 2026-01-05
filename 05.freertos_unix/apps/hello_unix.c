#include "console.h"
#include "kernel/syscall.h"
#include "FreeRTOS.h"
#include "task.h"

int app_hello_main(int argc, char** argv)
{
    (void)argc; (void)argv;
    const char* msg = "[HELLO] from unix-like app";
    sys_write(1, msg, (int)sizeof("[HELLO] from unix-like app")-1);
    sys_write(1, "\n", 1);
    int wait_count = 30;
    while(wait_count-- > 0) {
        // do nothing, just loop
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    return 0;
}

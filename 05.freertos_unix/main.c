#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "console.h"

/* -------- main -------- */
int main() {
    /* Initialize console (UART + buffers) before tasks */
    ConsoleInit();

    /* Console tasks */
    xTaskCreate(ConsoleRxTask,  "ConsoleRx",1024, NULL, 2, NULL);
    xTaskCreate(ConsoleTask,    "Console",  1024, NULL, 1, NULL);
    xTaskCreate(ConsoleTxTask,  "ConsoleTx",1024, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;);
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;);
}


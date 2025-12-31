#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define LED_PIN 25

/* IPC */
QueueHandle_t sensorQueue;
SemaphoreHandle_t eventSem;

/* -------- SensorTask -------- */
void SensorTask(void *arg) {
    TickType_t last = xTaskGetTickCount();
    int value = 0;

    while (1) {
        value++;
        xQueueSend(sensorQueue, &value, portMAX_DELAY);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(100));
    }
}

/* -------- LogicTask -------- */
void LogicTask(void *arg) {
    int value;

    while (1) {
        if (xQueueReceive(sensorQueue, &value, portMAX_DELAY)) {
            if ((value % 10) == 0) {
                xSemaphoreGive(eventSem);
            }
        }
    }
}

/* -------- OutputTask -------- */
void OutputTask(void *arg) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1) {
        xSemaphoreTake(eventSem, portMAX_DELAY);

        printf("### EVENT ###\n");

        gpio_put(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_put(LED_PIN, 0);
    }
}

/* -------- main -------- */
int freertos_basic() {

    sensorQueue = xQueueCreate(8, sizeof(int));
    eventSem    = xSemaphoreCreateBinary();

    configASSERT(sensorQueue);
    configASSERT(eventSem);

    xTaskCreate(SensorTask, "Sensor", 256, NULL, 1, NULL);
    xTaskCreate(LogicTask,  "Logic",  256, NULL, 2, NULL);
    xTaskCreate(OutputTask, "Output", 256, NULL, 1, NULL);

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


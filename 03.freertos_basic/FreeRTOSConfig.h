#pragma once
#include <stdint.h>

/* ---- 基本 ---- */
#define configUSE_PREEMPTION            1
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0

#define configCPU_CLOCK_HZ              (125000000)
#define configTICK_RATE_HZ              1000
#define configTICK_TYPE_WIDTH_IN_BITS   TICK_TYPE_WIDTH_32_BITS   // ★重要

#define configMAX_PRIORITIES            5
#define configMINIMAL_STACK_SIZE        128
#define configTOTAL_HEAP_SIZE           (20 * 1024)

/* ---- 同期 ---- */
#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   1

/* ---- デバッグ ---- */
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configUSE_MALLOC_FAILED_HOOK    1

#define configASSERT(x) \
    if (!(x)) { taskDISABLE_INTERRUPTS(); for(;;); }

/* ---- 有効API ---- */
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_vTaskDelayUntil             1
#define INCLUDE_xTaskGetTickCount_

#define configUSE_TIMERS                 1
#define configTIMER_TASK_PRIORITY        3
#define configTIMER_QUEUE_LENGTH         8
#define configTIMER_TASK_STACK_DEPTH     256

#define INCLUDE_xTimerPendFunctionCall        1
#define INCLUDE_xTimerPendFunctionCallFromISR 1
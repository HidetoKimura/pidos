/*
 * FreeRTOS-based console with UART RX ISR -> StreamBuffer,
 * ConsoleRxTask (line assembly), ConsoleTask (parse/dispatch),
 * ConsoleTxTask (serialized output).
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stream_buffer.h"

#include "commands.h"
#include "console.h"

/* UART selection: set via -DCONSOLE_UART_ID=0 or 1 */
#ifndef CONSOLE_UART_ID
#define CONSOLE_UART_ID 0
#endif

#if CONSOLE_UART_ID == 0
#define UART_ID      uart0
#define UART_IRQ     UART0_IRQ
#elif CONSOLE_UART_ID == 1
#define UART_ID      uart1
#define UART_IRQ     UART1_IRQ
#else
#error Invalid CONSOLE_UART_ID (use 0 or 1)
#endif

#define UART_BAUD          115200
#ifndef UART_TX_PIN
#define UART_TX_PIN        0
#endif
#ifndef UART_RX_PIN
#define UART_RX_PIN        1
#endif

#define INPUT_BUFFER_SIZE  256
#define RX_STREAM_SIZE     256
#define LINE_QUEUE_LEN     8

#define TX_MSG_MAX         128
#define TX_QUEUE_LEN       16

#define ARG_MAX            32
#define DELIM              " \t\n\r"

/* Buffers */
static StreamBufferHandle_t rxStream = NULL;                /* ISR -> bytes */
static QueueHandle_t        lineQueue = NULL;               /* lines -> parser */
static QueueHandle_t        txQueue = NULL;                 /* messages -> TX */
static QueueHandle_t        modalQueue = NULL;              /* lines -> modal consumer */
static volatile int         modalActive = 0;

/* --- Parser helper --- */
static void parse_input_stream(char* s, CommandList* cmd)
{
    int32_t     argc;
    uint32_t    i;
    int32_t     ret;
    char*       argv[ARG_MAX];
    char*       tok;

    tok = strtok(s, DELIM);
    if(tok == NULL) {
        return;
    }

    argv[0] = tok;
    for(i = 1; i < (sizeof(argv)/sizeof(argv[0])); i++) {
        if((tok = strtok(NULL, DELIM)) == NULL) break;
        argv[i] = tok;
    }
    argc = i;

    ret = -1;
    for(; cmd->command_func != NULL; cmd++) {
        if(strcmp(argv[0], cmd->command_name) == 0) {
            ret = cmd->command_func(argc, argv);
            if (ret){
                console_printf("error command:%s\nusage:%s \n", s, cmd->usage);
            }
            break;
        }
    }
    if (cmd->command_func == NULL) {
        console_printf("Unknown command: %s.\nType 'help' for options.\n", argv[0]);
    }
}

/* --- UART RX ISR: push bytes into StreamBuffer --- */
static void uart_rx_isr(void)
{
    BaseType_t xWoken = pdFALSE;
    /* Drain RX FIFO without using blocking SDK helper */
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = (uint8_t)(uart_get_hw(UART_ID)->dr & 0xFF);
        (void)xStreamBufferSendFromISR(rxStream, &ch, 1, &xWoken);
        /* Clear any latched RX errors (RSR/ECR alias) */
        uint32_t rsr = uart_get_hw(UART_ID)->rsr;
        if (rsr) {
            uart_get_hw(UART_ID)->rsr = 0; /* write clears errors */
        }
    }
    portYIELD_FROM_ISR(xWoken);
}

/* --- ConsoleInit: UART + buffers --- */
void ConsoleInit(void)
{
    /* Create buffers */
    rxStream   = xStreamBufferCreate(RX_STREAM_SIZE, 1);
    lineQueue  = xQueueCreate(LINE_QUEUE_LEN, INPUT_BUFFER_SIZE);
    txQueue    = xQueueCreate(TX_QUEUE_LEN, TX_MSG_MAX);
    modalQueue = xQueueCreate(2, INPUT_BUFFER_SIZE);

    configASSERT(rxStream);
    configASSERT(lineQueue);
    configASSERT(txQueue);
    configASSERT(modalQueue);

    /* Configure UART0 */
    uart_init(UART_ID, UART_BAUD);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_ID, true);

    /* RX interrupt */
    irq_set_exclusive_handler(UART_IRQ, uart_rx_isr);
    /* Set priority low enough to call FreeRTOS FromISR APIs */
    irq_set_priority(UART_IRQ, 0xC0);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);
}

/* --- RxTask: assemble lines with basic editing --- */
void ConsoleRxTask(void *arg)
{
    (void)arg;
    char line[INPUT_BUFFER_SIZE];
    size_t idx = 0;

    /* Prompt */
    console_printf("Pico Console Ready. Type Enter to run.\n");
    console_printf("> ");

    for (;;) {
        uint8_t ch;
        size_t n = xStreamBufferReceive(rxStream, &ch, 1, portMAX_DELAY);
        if (n == 1) {
            if (ch == '\r' || ch == '\n') {
                /* Finish line */
                line[idx] = '\0';
                /* Route line depending on mode */
                if (modalActive) {
                    xQueueSend(modalQueue, line, portMAX_DELAY);
                } else {
                    /* Always send (empty line triggers prompt) */
                    xQueueSend(lineQueue, line, portMAX_DELAY);
                }
                idx = 0;
            } else if (ch == 0x08 || ch == 0x7F) { /* Backspace */
                if (idx > 0) {
                    idx--;
                    /* Visual backspace */
                    if (!modalActive) console_printf("\b \b");
                }
            } else if (ch >= 32 && ch <= 126) { /* printable */
                if (idx < INPUT_BUFFER_SIZE - 1) {
                    line[idx++] = (char)ch;
                    /* Echo via formatted TX path */
                    if (!modalActive) console_printf("%c", (char)ch);
                } else {
                    /* Optional bell on overflow */
                    if (!modalActive) console_printf("\a");
                }
            }
        }
    }
}

/* --- ConsoleTask: parse + dispatch --- */
void ConsoleTask(void *arg)
{
    (void)arg;
    char line[INPUT_BUFFER_SIZE];
    for (;;) {
        if (xQueueReceive(lineQueue, line, portMAX_DELAY)) {
            /* Echo newline from Enter before handling */
            console_printf("\n");
            /* Empty line is allowed: prints prompt */
            if (line[0] != '\0') {
                parse_input_stream(line, commands);
            }
            /* Prompt after command output finishes */
            console_printf("> ");
        }
    }
}

/* --- TxTask: serialize output to UART --- */
void ConsoleTxTask(void *arg)
{
    (void)arg;
    char msg[TX_MSG_MAX];
    for (;;) {
        if (xQueueReceive(txQueue, msg, portMAX_DELAY)) {
            size_t len = strnlen(msg, TX_MSG_MAX);
            if (len > 0) {
                for (size_t i = 0; i < len; i++) {
                    char c = msg[i];
                    if (c == '\n') {
                        uart_putc_raw(UART_ID, '\r');
                        uart_putc_raw(UART_ID, '\n');
                    } else {
                        uart_putc_raw(UART_ID, c);
                    }
                }
            }
        }
    }
}

/* --- console_printf: format and enqueue --- */
int console_printf(const char *fmt, ...)
{
    char buf[TX_MSG_MAX];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    /* Split into chunks if longer */
    if (n <= 0) return n;
    size_t total = (size_t)n;
    size_t off = 0;
    while (off < total) {
        size_t chunk = total - off;
        if (chunk > TX_MSG_MAX - 1) chunk = TX_MSG_MAX - 1;
        char tmp[TX_MSG_MAX];
        memcpy(tmp, buf + off, chunk);
        tmp[chunk] = '\0';
        xQueueSend(txQueue, tmp, portMAX_DELAY);
        off += chunk;
    }
    return n;
}

int console_putc(int c)
{
    char ch = (char)c;
    console_printf("%c", ch);
    return (int)(unsigned char)ch;
}

int console_puts(const char *s)
{
    if (!s) return -1;
    console_printf("%s\n", s);
    return 0;
}

/* --- Modal input controls --- */
void console_modal_begin(void)
{
    modalActive = 1;
}

void console_modal_end(void)
{
    modalActive = 0;
}

int console_readline(char* buf, size_t max, uint32_t timeout_ms)
{
    if (!buf || max == 0) return -1;
    TickType_t to = (timeout_ms == (uint32_t)0xFFFFFFFFu)
                    ? portMAX_DELAY
                    : pdMS_TO_TICKS(timeout_ms);
    char tmp[INPUT_BUFFER_SIZE];
    if (xQueueReceive(modalQueue, tmp, to)) {
        size_t n = strnlen(tmp, INPUT_BUFFER_SIZE);
        if (n >= max) n = max - 1;
        memcpy(buf, tmp, n);
        buf[n] = '\0';
        return (int)n;
    }
    return 0; /* timeout */
}

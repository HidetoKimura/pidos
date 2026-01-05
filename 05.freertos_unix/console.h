#pragma once
#include <stddef.h>
#include <stdint.h>

/* Initialization of UART + buffers */
void ConsoleInit(void);

/* Tasks */
void ConsoleRxTask(void *arg);
void ConsoleTask(void *arg);
void ConsoleTxTask(void *arg);
void ConsoleStdioFeederTask(void *arg);

/* Thread-safe output function routed to ConsoleTxTask */
int console_printf(const char *fmt, ...);
int console_putc(int c);
int console_puts(const char *s);

/* Modal line input for interactive commands (e.g., loadhex) */
void console_modal_begin(void);
void console_modal_end(void);
int  console_readline(char* buf, size_t max, uint32_t timeout_ms);
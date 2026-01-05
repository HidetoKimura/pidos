// Comments must be in English.
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include "pico/stdlib.h"

#include "util.h"
#include "exec.h"
#include "rdisk.h"
#include "service_table.h"
#include "console.h"

static jmp_buf g_run_jmp;
static volatile int g_exit_code = 0;
static volatile int g_in_run = 0;

#define RUN_ADDR  (0x20020000u)

static void os_exit_impl(int code){
    g_exit_code = code;
    g_in_run = 0;
    longjmp(g_run_jmp, 1);      // Return to the caller here
}

static void service_table_init(void)
{
    svc_tbl()->puts = console_puts;
    svc_tbl()->exit = os_exit_impl;
}

static void mem_copy8(uint8_t* d, const uint8_t* s, uint32_t n)
{
    for (uint32_t i=0;i<n;i++) d[i]=s[i];
}

static int os_run_image(uint32_t load_addr)
{
    // Setup for longjmp return
    g_in_run = 1;
    g_exit_code = 0;

    if (setjmp(g_run_jmp) != 0) {
        // Returned via longjmp
        return g_exit_code;
    }

    // Thumb mode, so jump with bit0=1
    void (*entry)(void) = (void(*)(void))((load_addr | 1u));
    entry();

    // If entry returns (app that does not call exit)
    g_in_run = 0;
    return 0;
}

// "run NAME [ADDR]"
int32_t exec_cmd_run(int32_t argc, char **argv)
{
    if (argc < 2 || argc > 3){
        console_printf("Usage: run <name> [addrhex]\n");
        return -1;
    }

    const char* name = argv[1];
    uint32_t addr = (argc >= 3) ? parse_hex_u32(argv[2]) : (uint32_t)RUN_ADDR;

    void* p=0; 
    uint32_t len=0;

    if (rdisk_read_file(name, &p, &len) != 0){
        console_printf("No such file\n");
        return -1;
    }
    if (len == 0){
        console_printf("Empty file\n");
        return -1;
    }

    // Initialize service table
    service_table_init();

    // Copy
    mem_copy8((uint8_t*)addr, (const uint8_t*)p, len);

    console_printf("[RUN] %s at 0x%08X\n", name, addr);

    int rc = os_run_image(addr);

    console_printf("[RETURN] rc = %d\n", rc);

    return 0;
}


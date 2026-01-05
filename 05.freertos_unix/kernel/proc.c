#include <string.h>
#include "proc.h"
#include "queue.h"
#include "console.h"

typedef struct {
    const char*    name;
    app_entry_t    entry;
} builtin_app_t;

/* Builtins registry (defined later) */
extern int app_hello_main(int argc, char** argv);

static const builtin_app_t g_builtins[] = {
    {"hello", app_hello_main},
    {NULL, NULL},
};

static proc_t g_procs[NPROC];
static int    g_next_pid = 1;

/* Simple zombie notification */
typedef struct { int pid; int status; } zomb_t;
static QueueHandle_t zombQ;

void proc_init(void)
{
    memset(g_procs, 0, sizeof(g_procs));
    zombQ = xQueueCreate(8, sizeof(zomb_t));
}

static proc_t* alloc_slot(void)
{
    for (int i = 0; i < NPROC; i++) {
        if (!g_procs[i].used) {
            g_procs[i].used = 1;
            g_procs[i].pid = g_next_pid++;
            g_procs[i].state = P_READY;
            g_procs[i].exit_code = 0;
            memset(g_procs[i].fds, 0, sizeof(g_procs[i].fds));
            return &g_procs[i];
        }
    }
    return NULL;
}

proc_t* proc_get(int pid)
{
    for (int i = 0; i < NPROC; i++) if (g_procs[i].used && g_procs[i].pid == pid) return &g_procs[i];
    return NULL;
}

int proc_get_current_pid(void)
{
    TaskHandle_t self = xTaskGetCurrentTaskHandle();
    for (int i = 0; i < NPROC; i++) if (g_procs[i].used && g_procs[i].task == self) return g_procs[i].pid;
    return 0;
}

static const builtin_app_t* find_builtin(const char* name)
{
    for (int i = 0; g_builtins[i].name; i++) {
        if (strcmp(g_builtins[i].name, name) == 0) return &g_builtins[i];
    }
    return NULL;
}

typedef struct { proc_t* p; const builtin_app_t* b; int argc; char** argv; } run_ctx_t;

static void app_task(void* arg)
{
    run_ctx_t* rc = (run_ctx_t*)arg;
    console_printf("[app_task] start pid=%d name=%s\n", rc->p->pid, rc->b->name);
    rc->p->state = P_RUNNING;
    int code = rc->b->entry(rc->argc, rc->argv);
    rc->p->exit_code = code;
    rc->p->state = P_ZOMBIE;
    zomb_t z = { rc->p->pid, code };
    xQueueSend(zombQ, &z, portMAX_DELAY);
    console_printf("[app_task] exit pid=%d status=%d\n", rc->p->pid, code);
    vTaskDelete(NULL);
}

int proc_spawn_builtin(const char* name, int argc, char** argv, int parent_pid)
{
    const builtin_app_t* b = find_builtin(name);
    if (!b) return -1;
    proc_t* p = alloc_slot();
    if (!p) return -2;
    p->ppid = parent_pid;
    run_ctx_t* rc = (run_ctx_t*)pvPortMalloc(sizeof(run_ctx_t));
    if (!rc) return -3;
    rc->p = p; rc->b = b; rc->argc = argc; rc->argv = argv;
    BaseType_t ok = xTaskCreate(app_task, name, STACK_SIZE, rc, 2, &p->task);
    if (ok != pdPASS) {
        vPortFree(rc);
        p->used = 0;
        console_printf("[spawn] xTaskCreate failed for %s (err=%d)\n", name, (int)ok);
        return -4;
    }
    console_printf("[spawn] created pid=%d name=%s\n", p->pid, name);
    return p->pid;
}

int proc_wait_any(int* out_pid, int* out_status, TickType_t to)
{
    zomb_t z;
    if (xQueueReceive(zombQ, &z, to)) {
        proc_t* p = proc_get(z.pid);
        if (p) { p->used = 0; }
        if (out_pid) *out_pid = z.pid;
        if (out_status) *out_status = z.status;
        return 1;
    }
    return 0;
}

void proc_mark_exit(int pid, int status)
{
    zomb_t z = { pid, status };
    xQueueSend(zombQ, &z, portMAX_DELAY);
}

int proc_kill(int pid, int status)
{
    proc_t* p = proc_get(pid);
    if (!p || !p->used) return -1;
    if (p->state == P_ZOMBIE) return 0;
    p->exit_code = status;
    p->state = P_ZOMBIE;
    if (p->task) {
        vTaskDelete(p->task);
        p->task = NULL;
    }
    zomb_t z = { pid, status };
    xQueueSend(zombQ, &z, portMAX_DELAY);
    console_printf("[kill] pid=%d status=%d\n", pid, status);
    return 0;
}

void proc_list(void)
{
    console_printf("PID  PPID  STATE  EXIT\n");
    for (int i = 0; i < NPROC; i++) {
        if (g_procs[i].used) {
            const char* st = (g_procs[i].state==P_READY)?"READY":(g_procs[i].state==P_RUNNING)?"RUN":(g_procs[i].state==P_ZOMBIE)?"ZOMB":"?";
            console_printf("%3d  %4d  %4s  %4d\n", g_procs[i].pid, g_procs[i].ppid, st, g_procs[i].exit_code);
        }
    }
}

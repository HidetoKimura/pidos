#include <string.h>
#include "vfs.h"
#include "console.h"

static int tty_read(file_t* f, void* buf, int len)
{
    (void)f;
    char tmp[256];
    console_modal_begin();
    int n = console_readline(tmp, sizeof(tmp), 0xFFFFFFFFu);
    console_modal_end();
    if (n <= 0) return 0;
    if (n > len) n = len;
    memcpy(buf, tmp, n);
    return n;
}

static int tty_write(file_t* f, const void* buf, int len)
{
    (void)f;
    const char* s = (const char*)buf;
    for (int i = 0; i < len; i++) console_putc((int)(unsigned char)s[i]);
    return len;
}

static int tty_close(file_t* f) { (void)f; return 0; }

static file_ops_t tty_ops = { tty_read, tty_write, tty_close };
static file_t     dev_tty = { "/dev/tty", &tty_ops, NULL };

static int null_read(file_t* f, void* buf, int len) { (void)f; (void)buf; (void)len; return 0; }
static int null_write(file_t* f, const void* buf, int len) { (void)f; (void)buf; return len; }
static int null_close(file_t* f) { (void)f; return 0; }
static file_ops_t null_ops = { null_read, null_write, null_close };
static file_t     dev_null = { "/dev/null", &null_ops, NULL };

int vfs_init(void) { return 0; }

file_t* vfs_open(const char* path)
{
    if (strcmp(path, "/dev/tty") == 0) return &dev_tty;
    if (strcmp(path, "/dev/null") == 0) return &dev_null;
    return NULL;
}

int vfs_close(file_t* f) { if (!f) return -1; return f->ops->close(f); }
int vfs_read(file_t* f, void* buf, int len) { if (!f) return -1; return f->ops->read(f, buf, len); }
int vfs_write(file_t* f, const void* buf, int len) { if (!f) return -1; return f->ops->write(f, buf, len); }

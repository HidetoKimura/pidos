#include "pico/stdlib.h"
#include "dos/dos.h"
#include "vfs/vfs.h"
#include "fs/ramfs.h"
#include "fs/flash_fs.h"

int main(void) {
    stdio_init_all();

    vfs_init();

    ramfs_init();      // Provide A: drive in RAM
    if (!flash_fs_load()) {
        // On first run or corruption, keep initial RAMFS
    }

    dos_init();        // Register shell/apps, etc.

    dos_println("PicoDOS (educational) 0.1");
    dos_println("Type HELP.");

    dos_run();         // COMMAND loop

    while (1) { tight_loop_contents(); }
    return 0;
}

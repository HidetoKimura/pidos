#include "pico/stdlib.h"
#include "dos/dos.h"
#include "vfs/vfs.h"
#include "fs/ramfs.h"

int main(void) {
    stdio_init_all();

    vfs_init();
    ramfs_init();      // A: ドライブをRAMで用意
    dos_init();        // シェル/アプリ登録など

    dos_println("PicoDOS (educational) 0.1");
    dos_println("Type HELP.");

    dos_run();         // COMMAND loop

    while (1) { tight_loop_contents(); }
    return 0;
}

#include "xfer/xfer_recv.h"
#include "xfer/cobs.h"
#include "vfs/vfs.h"
#include "dos/dos.h"
#include "dos/dos_sys.h"
#include <string.h>
#include <stdint.h>

// UART input (replace per your implementation)
// Example: assume dos_getchar() returns 1 byte blocking
extern int dos_getchar(void);

static uint32_t crc32_simple(const uint8_t* p, size_t n) {
    uint32_t x = 0x12345678u;
    for (size_t i=0;i<n;i++) x = (x * 33u) ^ p[i];
    return x;
}

static bool read_frame(uint8_t* enc, size_t enc_cap, size_t* enc_len) {
    // Receive delimited by 0x00 (COBS frame)
    size_t n = 0;
    while (1) {
        int c = dos_getc_blocking();
        if (c < 0) return false;
        if (c == 0) break;              // end of frame
        if (n >= enc_cap) return false; // frame too big
        enc[n++] = (uint8_t)c;
    }
    *enc_len = n;
    return n > 0;
}

// little-endian helpers
static uint16_t rd16(const uint8_t* p){ return (uint16_t)p[0] | ((uint16_t)p[1]<<8); }
static uint32_t rd32(const uint8_t* p){ return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }

bool xfer_recv_file(const char* path) {
    // Receive buffers (tunable)
    static uint8_t enc[600];
    static uint8_t dec[520];

    dos_puts("Waiting BEGIN frame...\r\n");

    // BEGIN
    size_t enc_len=0;
    if (!read_frame(enc, sizeof(enc), &enc_len)) return false;
    size_t dec_len = cobs_decode(enc, enc_len, dec, sizeof(dec));
    if (dec_len < 1+4+2+4+4) { dos_puts("Bad BEGIN\r\n"); return false; }

    const uint8_t type = dec[0];
    const uint32_t seq = rd32(&dec[1]);
    if (type != 1 || seq != 0) { dos_puts("BEGIN mismatch\r\n"); return false; }

    const uint16_t name_len = rd16(&dec[5]);
    const uint32_t file_size = rd32(&dec[7]);
    const uint32_t expect_crc = rd32(&dec[11]);

    if (dec_len < (size_t)(15 + name_len)) { dos_puts("Bad BEGIN len\r\n"); return false; }

    // Received name is for logging (use RECV arg path)
    // const uint8_t* name = &dec[15];

    vfs_err_t e;
    int fd = vfs_open(path, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC, &e);
    if (fd < 0) { dos_puts("Cannot open file\r\n"); return false; }

    dos_puts("Receiving...\r\n");

    uint32_t got_total = 0;
    uint32_t crc_acc = 0x12345678u;
    uint32_t next_seq = 1;

    while (1) {
        if (!read_frame(enc, sizeof(enc), &enc_len)) { dos_puts("RX frame error\r\n"); vfs_close(fd); return false; }
        dec_len = cobs_decode(enc, enc_len, dec, sizeof(dec));
        if (dec_len < 1+4) { dos_puts("Bad frame\r\n"); vfs_close(fd); return false; }

        uint8_t t = dec[0];
        uint32_t s = rd32(&dec[1]);

        if (t == 2) { // DATA
            if (dec_len < 1+4+2) { dos_puts("Bad DATA\r\n"); vfs_close(fd); return false; }
            if (s != next_seq) { dos_puts("SEQ mismatch\r\n"); vfs_close(fd); return false; }
            uint16_t chunk_len = rd16(&dec[5]);
            if (dec_len < (size_t)(7 + chunk_len)) { dos_puts("Bad chunk\r\n"); vfs_close(fd); return false; }

            const uint8_t* chunk = &dec[7];
            int w = vfs_write(fd, chunk, chunk_len, &e);
            if (w != (int)chunk_len) { dos_puts("Write error\r\n"); vfs_close(fd); return false; }

            // CRC (simple)
            for (uint16_t i=0;i<chunk_len;i++) crc_acc = (crc_acc * 33u) ^ chunk[i];

            got_total += chunk_len;
            next_seq++;

            if (got_total > file_size) { dos_puts("Size overflow\r\n"); vfs_close(fd); return false; }
        }
        else if (t == 3) { // END
            // END is expected with seq = next_seq (optional)
            (void)s;
            break;
        }
        else {
            dos_puts("Unknown type\r\n");
            vfs_close(fd);
            return false;
        }
    }

    vfs_close(fd);

    if (got_total != file_size) { dos_puts("Size mismatch\r\n"); return false; }
    if (crc_acc != expect_crc) { dos_puts("CRC mismatch\r\n"); return false; }

    dos_puts("OK\r\n");
    return true;
}

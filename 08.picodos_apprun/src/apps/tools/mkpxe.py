#!/usr/bin/env python3
import struct, sys, pathlib

PXE_MAGIC = 0x30584550  # 'PXE0'
PXE_VER = 1

def main():
    if len(sys.argv) < 4:
        print("Usage: mkpxe.py <input.bin> <output.pxe> <entry_off_hex> [bss_size_hex]")
        sys.exit(1)

    inbin = pathlib.Path(sys.argv[1]).read_bytes()
    outpxe = sys.argv[2]
    entry_off = int(sys.argv[3], 16)
    bss_size = int(sys.argv[4], 16) if len(sys.argv) >= 5 else 0

    hdr = struct.pack("<IHHIII3I",
                      PXE_MAGIC, PXE_VER, 0,
                      len(inbin), bss_size, entry_off,
                      0,0,0)

    pathlib.Path(outpxe).write_bytes(hdr + inbin)
    print(f"Wrote {outpxe}: image={len(inbin)} entry_off=0x{entry_off:x} bss=0x{bss_size:x}")

if __name__ == "__main__":
    main()

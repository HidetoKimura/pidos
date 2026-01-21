#!/usr/bin/env python3
import pathlib, re, struct, subprocess, sys

PXE_MAGIC = 0x30584550  # 'PXE0'
PXE_VER   = 1
BASE      = 0x20020000  # match app.ld
SYMBOL    = "app_entry"

def write_pxe(bin_path: pathlib.Path, out_pxe: pathlib.Path, entry_off: int, bss_size: int = 0):
    data = bin_path.read_bytes()
    hdr = struct.pack("<IHHIII3I",
                      PXE_MAGIC, PXE_VER, 0,
                      len(data), bss_size, entry_off,
                      0, 0, 0)
    out_pxe.write_bytes(hdr + data)

def main():
    if len(sys.argv) != 4:
        print("Usage: mkpxe_from_elf.py <app.elf> <out.pxe> <objcopy_prefix>", file=sys.stderr)
        print("  objcopy_prefix example: arm-none-eabi", file=sys.stderr)
        sys.exit(1)

    elf = pathlib.Path(sys.argv[1])
    out_pxe = pathlib.Path(sys.argv[2])
    prefix = sys.argv[3]
    objcopy = f"{prefix}-objcopy"
    nm      = f"{prefix}-nm"

    bin_path = elf.with_suffix(".bin")

    # ELF -> BIN
    subprocess.check_call([objcopy, "-O", "binary", str(elf), str(bin_path)])

    # Get app_entry address via nm
    nm_out = subprocess.check_output([nm, "-n", str(elf)], text=True, errors="ignore")
    addr = None
    for line in nm_out.splitlines():
        m = re.match(r"^([0-9a-fA-F]+)\s+\w\s+" + re.escape(SYMBOL) + r"$", line.strip())
        if m:
            addr = int(m.group(1), 16)
            break
    if addr is None:
        print("app_entry not found in nm output", file=sys.stderr)
        sys.exit(2)

    entry_off = addr - BASE
    write_pxe(bin_path, out_pxe, entry_off, 0)
    print(f"Wrote {out_pxe}: image={bin_path.stat().st_size} entry_off=0x{entry_off:x} bss=0x0")

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
import sys, re, pathlib, subprocess

BASE = 0x20020000  # match app.ld
SYMBOL = "app_entry"

def main():
    if len(sys.argv) != 4:
        print("Usage: mkpxe_gen.py <in.bin> <out.pxe> <nm.txt>")
        sys.exit(1)

    inbin, outpxe, nmtxt = sys.argv[1], sys.argv[2], sys.argv[3]
    nm = pathlib.Path(nmtxt).read_text(errors="ignore").splitlines()

    addr = None
    for line in nm:
        # "20020054 T app_entry"
        m = re.match(r"^([0-9a-fA-F]+)\s+\w\s+" + re.escape(SYMBOL) + r"$", line.strip())
        if m:
            addr = int(m.group(1), 16)
            break
    if addr is None:
        print("app_entry not found in nm")
        sys.exit(2)

    entry_off = addr - BASE
    # call mkpxe.py
    subprocess.check_call([sys.executable, str(pathlib.Path(__file__).with_name("mkpxe.py")),
                           inbin, outpxe, hex(entry_off), "0x0"])
    print(f"entry_off={hex(entry_off)}")

if __name__ == "__main__":
    main()

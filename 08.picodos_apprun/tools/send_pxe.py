#!/usr/bin/env python3
import serial, struct, sys, time

def cobs_encode(data: bytes) -> bytes:
    out = bytearray()
    idx = 0
    while idx < len(data):
        code_pos = len(out)
        out.append(0)  # placeholder
        code = 1
        while idx < len(data) and data[idx] != 0 and code < 0xFF:
            out.append(data[idx])
            idx += 1
            code += 1
        out[code_pos] = code
        if idx < len(data) and data[idx] == 0:
            idx += 1
    if data and data[-1] == 0:
        out.append(1)
    return bytes(out)

def crc32_simple(data: bytes) -> int:
    x = 0x12345678
    for b in data:
        x = (x * 33) ^ b
        x &= 0xFFFFFFFF
    return x

def write_frame(ser, payload: bytes):
    enc = cobs_encode(payload)
    ser.write(enc + b"\x00")  # delimiter
    ser.flush()

def main():
    if len(sys.argv) != 4:
        print("Usage: send_pxe.py <port> <file.pxe> <remote_name>", file=sys.stderr)
        print("Example: send_pxe.py /dev/ttyACM0 HELLO.PXE A:\\HELLO.PXE", file=sys.stderr)
        sys.exit(1)

    port, path, remote = sys.argv[1], sys.argv[2], sys.argv[3]
    data = open(path, "rb").read()
    total = len(data)
    crc = crc32_simple(data)

    name_bytes = remote.encode("ascii", errors="strict")
    if len(name_bytes) > 120:
        raise SystemExit("remote name too long")

    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(0.2)

    # BEGIN: type=1, seq=0
    begin = struct.pack("<B I H I I", 1, 0, len(name_bytes), total, crc) + name_bytes
    write_frame(ser, begin)

    # DATA: type=2, seq=1.., chunk 240 bytes (COBS overhead still fits comfortably)
    seq = 1
    chunk_size = 240
    off = 0
    while off < total:
        chunk = data[off:off+chunk_size]
        pkt = struct.pack("<B I H", 2, seq, len(chunk)) + chunk
        write_frame(ser, pkt)
        off += len(chunk)
        seq += 1

    # END: type=3
    end = struct.pack("<B I", 3, seq)
    write_frame(ser, end)

    print(f"sent {total} bytes, frames={seq+1}")

if __name__ == "__main__":
    main()

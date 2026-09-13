import struct
import sys
import time
import serial

PORT = "COM15"
BAUD = 115200

SOF = 0xA5
MAGIC = bytes([0xA5, 0x5A, 0xB0, 0x07])
MAGIC_ACK = 0x79

CMD_GET_INFO = 0x01
CMD_ERASE    = 0x02
CMD_WRITE    = 0x03
CMD_VERIFY   = 0x04

RESP_ACK = 0x79
RESP_NAK = 0x1F

APP_SECTOR = 5
CHUNK = 128

NAK_REASONS = {
    0x01: "bad crc",
    0x02: "bad length",
    0x03: "unknown command",
    0x04: "flash error",
    0x05: "verify failed",
}


def crc32_mpeg2(data):
    if len(data) % 4:
        data = data + b"\x00" * (4 - len(data) % 4)
    crc = 0xFFFFFFFF
    for b in data:
        crc ^= b << 24
        for _ in range(8):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    return crc


def build_frame(cmd, payload=b""):
    length = len(payload) + 1
    body = bytes([SOF]) + struct.pack("<H", length) + bytes([cmd]) + payload
    return body + struct.pack("<I", crc32_mpeg2(body))


def read_frame(ser):
    while True:
        b = ser.read(1)
        if not b:
            raise TimeoutError("no response")
        if b[0] == SOF:
            break

    hdr = ser.read(2)
    length = struct.unpack("<H", hdr)[0]
    body = ser.read(length)
    crc_bytes = ser.read(4)

    if len(body) != length or len(crc_bytes) != 4:
        raise IOError("short frame")

    full = bytes([SOF]) + hdr + body
    if crc32_mpeg2(full) != struct.unpack("<I", crc_bytes)[0]:
        raise IOError("response crc mismatch")

    return body[0], body[1:]


def cmd(ser, c, payload=b""):
    ser.write(build_frame(c, payload))
    resp, data = read_frame(ser)
    if resp == RESP_NAK:
        raise IOError("NAK: " + NAK_REASONS.get(data[0], f"0x{data[0]:02X}"))
    if resp != RESP_ACK:
        raise IOError(f"unexpected 0x{resp:02X}")
    return data


def handshake(ser, seconds=15):
    print("press RESET on the board...", flush=True)
    ser.reset_input_buffer()
    deadline = time.time() + seconds

    while time.time() < deadline:
        ser.write(MAGIC)
        b = ser.read(1)
        if b and b[0] == MAGIC_ACK:
            print("bootloader responded")
            return True
        time.sleep(0.01)

    return False


def flash_image(ser, path):
    image = open(path, "rb").read()
    if len(image) % 4:
        image += b"\xFF" * (4 - len(image) % 4)

    print(f"image {len(image)} bytes")

    print("erasing...", end=" ", flush=True)
    cmd(ser, CMD_ERASE, bytes([APP_SECTOR]))
    print("ok")

    for off in range(0, len(image), CHUNK):
        block = image[off:off + CHUNK]
        cmd(ser, CMD_WRITE, struct.pack("<I", off) + block)
        pct = 100 * (off + len(block)) // len(image)
        print(f"\rwriting... {pct}%", end="", flush=True)
    print()

    crc = crc32_mpeg2(image)
    print(f"verifying {crc:08X}...", end=" ", flush=True)
    cmd(ser, CMD_VERIFY, struct.pack("<II", len(image), crc))
    print("ok")

    print("done - press RESET to run the new app")


def main():
    if len(sys.argv) < 2:
        print("usage: python host.py app.bin")
        return

    ser = serial.Serial(PORT, BAUD, timeout=0.05)

    if not handshake(ser):
        print("no bootloader - did you press reset?")
        ser.close()
        return

    ser.timeout = 3
    flash_image(ser, sys.argv[1])
    ser.close()


if __name__ == "__main__":
    main()
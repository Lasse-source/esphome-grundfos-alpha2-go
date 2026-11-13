#!/usr/bin/env python3
"""Extract ONLY the GENI frames exchanged with one Bluetooth device from an Android btsnoop_hci.log.

Why: a raw HCI capture contains your pairing keys (the LTK is visible in the 'LE Start Encryption' command) and
advertisements from every device nearby. Never publish the raw file. This script prints only the application
frames to/from the pump, one per line:

    S <hex>   phone -> pump
    R <hex>   pump -> phone

Usage:  python3 extract_geni_frames.py btsnoop_hci.log --mac AA:BB:CC:DD:EE:FF [--handle 0x001a]
"""
import argparse
import struct
import sys


def read_records(path):
    data = open(path, "rb").read()
    if data[:8] != b"btsnoop\x00":
        sys.exit("not a btsnoop file")
    pos = 16
    while pos + 24 <= len(data):
        orig_len, incl_len, flags, _drops, _ts = struct.unpack(">IIIIq", data[pos:pos + 24])
        pos += 24
        yield flags, data[pos:pos + incl_len]
        pos += incl_len


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("--mac", required=True, help="pump Bluetooth address, e.g. AA:BB:CC:DD:EE:FF")
    ap.add_argument("--handle", default="0x001a", help="GATT value handle of the GENI characteristic")
    args = ap.parse_args()
    mac = bytes(int(x, 16) for x in args.mac.split(":"))[::-1]
    gatt_handle = int(args.handle, 16)

    conn = None            # HCI connection handle of the pump link
    l2 = {}                # reassembly buffers per direction
    streams = {"S": bytearray(), "R": bytearray()}
    out = []

    for flags, p in read_records(args.log):
        if not p:
            continue
        # LE (Enhanced) Connection Complete: remember which HCI handle belongs to the pump
        if p[0] == 4 and p[1] == 0x3E and p[3] in (0x01, 0x0A) and p[4] == 0x00:
            if p[9:15] == mac:
                conn = struct.unpack("<H", p[5:7])[0] & 0x0FFF
            continue
        if p[0] != 2 or conn is None:
            continue
        hf, ln = struct.unpack("<HH", p[1:5])
        if (hf & 0x0FFF) != conn:
            continue
        pb = (hf >> 12) & 3
        d = "S" if (flags & 1) == 0 else "R"
        body = p[5:5 + ln]
        if pb in (0, 2):
            l2len, cid = struct.unpack("<HH", body[:4])
            l2[d] = [cid, l2len, bytearray(body[4:])]
        elif pb == 1 and d in l2:
            l2[d][2] += body
        else:
            continue
        cid, l2len, pl = l2[d]
        if len(pl) < l2len:
            continue
        del l2[d]
        if cid != 4:
            continue
        att = bytes(pl[:l2len])
        if len(att) < 3 or struct.unpack("<H", att[1:3])[0] != gatt_handle:
            continue
        if (d == "S" and att[0] == 0x52) or (d == "R" and att[0] == 0x1B):
            s = streams[d]
            s += att[3:]
            while len(s) >= 2:
                if s[0] not in (0x24, 0x27):
                    s.pop(0)
                    continue
                total = s[1] + 4
                if len(s) < total:
                    break
                out.append("%s %s" % (d, bytes(s[:total]).hex()))
                del s[:total]
    if conn is None:
        sys.exit("no connection to %s found in the capture" % args.mac)
    print("\n".join(out))
    print("# %d requests, %d replies" % (sum(o[0] == "S" for o in out), sum(o[0] == "R" for o in out)), file=sys.stderr)


if __name__ == "__main__":
    main()
